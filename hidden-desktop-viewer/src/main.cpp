#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include "desktop_manager.h"
#include "capture_manager.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// Forward-declare imgui Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ---------------------------------------------------------------------------
// D3D11 globals
// ---------------------------------------------------------------------------
static ID3D11Device*            g_pd3dDevice          = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext    = nullptr;
static IDXGISwapChain*          g_pSwapChain          = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;

static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();
static LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);

// ---------------------------------------------------------------------------
// Preview texture (BGRA → D3D11 Texture2D → SRV for ImGui::Image)
// ---------------------------------------------------------------------------
struct PreviewTexture {
    ID3D11Texture2D*          texture = nullptr;
    ID3D11ShaderResourceView* srv     = nullptr;
    int width = 0, height = 0;

    void Release() {
        if (srv)     { srv->Release();     srv     = nullptr; }
        if (texture) { texture->Release(); texture = nullptr; }
        width = height = 0;
    }

    void Update(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                const uint8_t* data, int w, int h)
    {
        if (w != width || h != height) {
            Release();
            D3D11_TEXTURE2D_DESC td{};
            td.Width            = static_cast<UINT>(w);
            td.Height           = static_cast<UINT>(h);
            td.MipLevels        = 1;
            td.ArraySize        = 1;
            td.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
            td.SampleDesc.Count = 1;
            td.Usage            = D3D11_USAGE_DEFAULT;
            td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA init{};
            init.pSysMem     = data;
            init.SysMemPitch = static_cast<UINT>(w * 4);

            if (SUCCEEDED(dev->CreateTexture2D(&td, &init, &texture))) {
                D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
                sd.Format                    = td.Format;
                sd.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
                sd.Texture2D.MipLevels       = 1;
                dev->CreateShaderResourceView(texture, &sd, &srv);
            }
            width  = w;
            height = h;
        } else if (texture) {
            ctx->UpdateSubresource(texture, 0, nullptr, data, w * 4, 0);
        }
    }
};

// ---------------------------------------------------------------------------
// A single capture window (created on double-click, closed by user)
// ---------------------------------------------------------------------------
struct CaptureItem {
    CaptureManager capture;
    PreviewTexture preview;
    CapturedFrame  frame;
    std::string    title;
    int            uid  = 0;
    bool           open = true;
};

// ---------------------------------------------------------------------------
// Application state
// ---------------------------------------------------------------------------
struct AppState {
    std::vector<DesktopInfo> desktops;
    int selectedDesktopIdx = -1;

    std::vector<WindowInfo>  windows;
    int selectedWindowIdx  = -1;

    std::vector<std::unique_ptr<CaptureItem>> items;
    int nextUid = 0;

    void RefreshDesktops() {
        desktops = DesktopManager::EnumerateDesktops();
        selectedDesktopIdx = -1;
        windows.clear();
        selectedWindowIdx = -1;
    }

    void RefreshWindows() {
        if (selectedDesktopIdx < 0 ||
            selectedDesktopIdx >= static_cast<int>(desktops.size()))
        {
            windows.clear();
            return;
        }
        windows = DesktopManager::EnumerateVisibleWindows(
                      desktops[selectedDesktopIdx].handle);
        selectedWindowIdx = -1;
    }
};

// ---------------------------------------------------------------------------
// WinMain
// ---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInstance;
    wc.lpszClassName  = L"HiddenDesktopViewer";
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowExW(
        0, wc.lpszClassName, L"Hidden Desktop Viewer",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1440, 900,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // ---- imgui setup ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding  = 2.0f;
    style.GrabRounding   = 2.0f;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load CJK-capable font (Microsoft YaHei) if available
    const char* fontPath = "C:\\Windows\\Fonts\\msyh.ttc";
    if (GetFileAttributesA(fontPath) != INVALID_FILE_ATTRIBUTES) {
        io.Fonts->AddFontFromFileTTF(fontPath, 16.0f, nullptr,
                                     io.Fonts->GetGlyphRangesChineseFull());
    }

    AppState app;
    app.RefreshDesktops();

    // ---- main loop ----
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // Handle resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight,
                                        DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Poll every active capture item
        for (auto& item : app.items) {
            if (item->capture.IsRunning() &&
                item->capture.GetLatestFrame(item->frame))
            {
                item->preview.Update(g_pd3dDevice, g_pd3dDeviceContext,
                                     item->frame.pixels.data(),
                                     item->frame.width,
                                     item->frame.height);
            }
        }

        // ---- new imgui frame ----
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ======================== UI ========================

        // Menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) done = true;
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        // ---------- Left panel: desktops & windows ----------
        ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 860), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Desktops & Windows")) {
            if (ImGui::Button("Refresh"))
                app.RefreshDesktops();

            ImGui::Separator();

            auto wsName = DesktopManager::GetWindowStationName();
            ImGui::Text("Station: %s",
                        DesktopManager::ToUTF8(wsName).c_str());
            ImGui::Separator();

            // Desktop tree
            for (int i = 0; i < static_cast<int>(app.desktops.size()); ++i) {
                auto& desk = app.desktops[i];
                std::string label = DesktopManager::ToUTF8(desk.name);
                bool isSel = (i == app.selectedDesktopIdx);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
                if (isSel) flags |= ImGuiTreeNodeFlags_Selected;

                bool open = ImGui::TreeNodeEx(
                    reinterpret_cast<void*>(static_cast<intptr_t>(i)),
                    flags, "%s", label.c_str());

                if (ImGui::IsItemClicked()) {
                    app.selectedDesktopIdx = i;
                    app.RefreshWindows();
                }

                // Double-click desktop → open a capture window
                if (ImGui::IsItemHovered() &&
                    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    app.selectedDesktopIdx = i;
                    app.RefreshWindows();
                    auto ci = std::make_unique<CaptureItem>();
                    ci->title = "Desktop: " + label;
                    ci->uid   = app.nextUid++;
                    ci->capture.StartDesktopCapture(desk.handle);
                    app.items.push_back(std::move(ci));
                }

                if (open) {
                    if (i == app.selectedDesktopIdx) {
                        for (int j = 0;
                             j < static_cast<int>(app.windows.size()); ++j)
                        {
                            auto& win = app.windows[j];
                            std::string wl =
                                DesktopManager::ToUTF8(win.title);
                            if (wl.empty()) wl = "(no title)";

                            bool ws = (j == app.selectedWindowIdx);
                            std::string sid = wl + "##w" + std::to_string(j);
                            if (ImGui::Selectable(sid.c_str(), ws,
                                    ImGuiSelectableFlags_AllowDoubleClick))
                            {
                                app.selectedWindowIdx = j;

                                // Double-click window → open a capture window
                                if (ImGui::IsMouseDoubleClicked(
                                        ImGuiMouseButton_Left))
                                {
                                    auto ci = std::make_unique<CaptureItem>();
                                    ci->title = "Window: " + wl;
                                    ci->uid   = app.nextUid++;
                                    ci->capture.StartWindowCapture(
                                        app.desktops[app.selectedDesktopIdx].handle,
                                        win.handle);
                                    app.items.push_back(std::move(ci));
                                }
                            }

                            if (ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ImGui::Text("HWND: 0x%p", win.handle);
                                ImGui::Text("Class: %s",
                                    DesktopManager::ToUTF8(win.className).c_str());
                                ImGui::Text("Rect: %d,%d  %dx%d",
                                    win.rect.left, win.rect.top,
                                    win.rect.right  - win.rect.left,
                                    win.rect.bottom - win.rect.top);
                                ImGui::EndTooltip();
                            }
                        }
                    } else {
                        ImGui::TextDisabled("(select desktop to list windows)");
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();

        // ---------- Dynamic capture windows ----------
        for (auto& item : app.items) {
            if (!item->open) continue;
            std::string winTitle = item->title
                + "###cap" + std::to_string(item->uid);
            ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);
            if (ImGui::Begin(winTitle.c_str(), &item->open)) {
                if (item->preview.srv) {
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    float srcA = static_cast<float>(item->preview.width)
                               / static_cast<float>(item->preview.height);
                    float dstA = avail.x / avail.y;
                    ImVec2 sz;
                    if (srcA > dstA) {
                        sz.x = avail.x;
                        sz.y = avail.x / srcA;
                    } else {
                        sz.y = avail.y;
                        sz.x = avail.y * srcA;
                    }
                    ImGui::Image(
                        reinterpret_cast<ImTextureID>(item->preview.srv), sz);
                    ImGui::Text("Resolution: %d x %d",
                                item->preview.width, item->preview.height);
                }
            }
            ImGui::End();
        }

        // Remove closed capture windows
        app.items.erase(
            std::remove_if(app.items.begin(), app.items.end(),
                [](std::unique_ptr<CaptureItem>& item) {
                    if (!item->open) {
                        item->capture.Stop();
                        item->preview.Release();
                        return true;
                    }
                    return false;
                }),
            app.items.end());

        // ======================== Render ========================
        ImGui::Render();
        const float clear[4] = { 0.10f, 0.10f, 0.12f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);   // vsync
    }

    // ---- cleanup ----
    for (auto& item : app.items) {
        item->capture.Stop();
        item->preview.Release();
    }
    app.items.clear();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

// ---------------------------------------------------------------------------
// D3D11 helpers
// ---------------------------------------------------------------------------
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags            = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow     = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed         = TRUE;
    sd.SwapEffect       = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    D3D_FEATURE_LEVEL got;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &got, &g_pd3dDeviceContext);
    if (hr == DXGI_ERROR_UNSUPPORTED)
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
            levels, 2, D3D11_SDK_VERSION, &sd,
            &g_pSwapChain, &g_pd3dDevice, &got, &g_pd3dDeviceContext);
    if (FAILED(hr)) return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain)       { g_pSwapChain->Release();       g_pSwapChain       = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)       { g_pd3dDevice->Release();       g_pd3dDevice       = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBack = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBack));
    if (pBack) {
        g_pd3dDevice->CreateRenderTargetView(pBack, nullptr,
                                             &g_mainRenderTargetView);
        pBack->Release();
    }
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth  = LOWORD(lParam);
        g_ResizeHeight = HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
