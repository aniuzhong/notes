#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using Microsoft::WRL::ComPtr;

namespace {

// ─── HLSL shaders (compiled at runtime) ───────────────────────────────────────
const char* kVertHLSL = R"(
struct VSInput  { float3 pos : POSITION; };
struct VSOutput { float4 pos : SV_POSITION; };
VSOutput main(VSInput input) {
    VSOutput output;
    output.pos = float4(input.pos, 1.0);
    return output;
}
)";

const char* kPixHLSL = R"(
cbuffer TimeCB : register(b0) { float uTime; };
float4 main() : SV_TARGET {
    return float4(0.5 + 0.5 * sin(uTime), 0.3, 0.7 + 0.3 * cos(uTime * 0.7), 1.0);
}
)";

// ─── Win32 globals ───────────────────────────────────────────────────────────
constexpr wchar_t kWndClass[] = L"D3D12TriangleWnd";
constexpr wchar_t kWndTitle[] = L"Win32 D3D12 Window";

HWND   g_hwnd      = nullptr;
int    g_width     = 800;
int    g_height    = 600;
bool   g_closing   = false;

// ─── D3D12 / DXGI globals ────────────────────────────────────────────────────
static constexpr UINT kFrameCount = 2;

ComPtr<ID3D12Device>             g_device;
ComPtr<ID3D12CommandQueue>       g_cmdQueue;
ComPtr<IDXGISwapChain3>          g_swapchain;
ComPtr<ID3D12DescriptorHeap>     g_rtvHeap;
UINT                             g_rtvSize = 0;
ComPtr<ID3D12Resource>           g_renderTargets[kFrameCount];

ComPtr<ID3D12RootSignature>      g_rootSig;
ComPtr<ID3D12PipelineState>      g_pso;
ComPtr<ID3D12Resource>           g_vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW         g_vbView = {};

ComPtr<ID3D12CommandAllocator>    g_cmdAlloc;
ComPtr<ID3D12GraphicsCommandList> g_cmdList;
ComPtr<ID3D12Fence>               g_fence;
UINT64                            g_fenceVal = 0;
HANDLE                            g_fenceEvt = nullptr;

// ─── helpers ──────────────────────────────────────────────────────────────────
void wait_for_gpu() {
    if (g_fence->GetCompletedValue() < g_fenceVal) {
        g_fence->SetEventOnCompletion(g_fenceVal, g_fenceEvt);
        WaitForSingleObject(g_fenceEvt, INFINITE);
    }
}

[[noreturn]] void fatal(const char* msg) {
    fprintf(stderr, "%s\n", msg);
    std::exit(EXIT_FAILURE);
}

ComPtr<ID3DBlob> compile_shader(const char* src, const char* entry, const char* target) {
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> err;
    HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
                            entry, target, 0, 0, blob.GetAddressOf(), err.GetAddressOf());
    if (FAILED(hr)) {
        if (err)
            fprintf(stderr, "Shader compile error:\n%s\n", (const char*)err->GetBufferPointer());
        else
            fprintf(stderr, "D3DCompile failed (HRESULT 0x%08lx)\n", (unsigned long)hr);
        std::exit(EXIT_FAILURE);
    }
    return blob;
}

// ─── swapchain-dependent resources ────────────────────────────────────────────
void destroy_swapchain_dependent() {
    for (UINT i = 0; i < kFrameCount; ++i)
        g_renderTargets[i].Reset();
}

void create_swapchain_dependent() {
    destroy_swapchain_dependent();
    for (UINT i = 0; i < kFrameCount; ++i) {
        g_swapchain->GetBuffer(i, IID_PPV_ARGS(g_renderTargets[i].GetAddressOf()));
        D3D12_CPU_DESCRIPTOR_HANDLE h = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        h.ptr += i * g_rtvSize;
        g_device->CreateRenderTargetView(g_renderTargets[i].Get(), nullptr, h);
    }
}

// ─── init functions ───────────────────────────────────────────────────────────

void init_window(HINSTANCE hInstance) {
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = [](HWND h, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
        switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            g_closing = true;
            PostQuitMessage(0);
            return 0;
        case WM_SIZE: {
            g_width  = LOWORD(lp);
            g_height = HIWORD(lp);
            if (g_device && g_swapchain && wp != SIZE_MINIMIZED) {
                wait_for_gpu();
                destroy_swapchain_dependent();
                g_swapchain->ResizeBuffers(kFrameCount, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
                create_swapchain_dependent();
            }
            return 0;
        }}
        return DefWindowProcW(h, msg, wp, lp);
    };
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kWndClass;
    if (!RegisterClassExW(&wc))
        fatal("RegisterClassExW failed");

    RECT r = {0, 0, g_width, g_height};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindowExW(0, kWndClass, kWndTitle, WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             r.right - r.left, r.bottom - r.top,
                             nullptr, nullptr, hInstance, nullptr);
    if (!g_hwnd)
        fatal("CreateWindowExW failed");

    ShowWindow(g_hwnd, SW_SHOW);
}

void init_d3d12() {
    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(factory.GetAddressOf()))))
        fatal("CreateDXGIFactory2 failed");

    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
                                  IID_PPV_ARGS(g_device.GetAddressOf()))))
        fatal("D3D12CreateDevice failed (no D3D12-capable GPU?)");

    D3D12_COMMAND_QUEUE_DESC cqDesc = {};
    cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(g_device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(g_cmdQueue.GetAddressOf()))))
        fatal("CreateCommandQueue failed");

    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width              = g_width;
    scDesc.Height             = g_height;
    scDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.SampleDesc.Count   = 1;
    scDesc.BufferCount        = kFrameCount;
    scDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    ComPtr<IDXGISwapChain1> sc1;
    if (FAILED(factory->CreateSwapChainForHwnd(g_cmdQueue.Get(), g_hwnd, &scDesc,
                                               nullptr, nullptr, sc1.GetAddressOf())))
        fatal("CreateSwapChainForHwnd failed");
    sc1.As(&g_swapchain);

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = kFrameCount;
    if (FAILED(g_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(g_rtvHeap.GetAddressOf()))))
        fatal("CreateDescriptorHeap (RTV) failed");
    g_rtvSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    create_swapchain_dependent();

    if (FAILED(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(g_fence.GetAddressOf()))))
        fatal("CreateFence failed");
    g_fenceEvt = CreateEventW(nullptr, FALSE, FALSE, nullptr);
}

void init_pipeline() {
    // Root signature: 1 root constant (float time) visible to pixel shader
    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParam.ShaderVisibility         = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParam.Constants.Num32BitValues = 1;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = 1;
    rsDesc.pParameters   = &rootParam;
    rsDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> sigBlob;
    ComPtr<ID3DBlob> errBlob;
    if (FAILED(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                            sigBlob.GetAddressOf(), errBlob.GetAddressOf()))) {
        if (errBlob)
            fprintf(stderr, "Root sig error: %s\n", (const char*)errBlob->GetBufferPointer());
        fatal("SerializeRootSignature failed");
    }
    if (FAILED(g_device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
                                             IID_PPV_ARGS(g_rootSig.GetAddressOf()))))
        fatal("CreateRootSignature failed");

    auto vsBlob = compile_shader(kVertHLSL, "main", "vs_5_0");
    auto psBlob = compile_shader(kPixHLSL, "main", "ps_5_0");

    D3D12_INPUT_ELEMENT_DESC inputElem = {};
    inputElem.SemanticName      = "POSITION";
    inputElem.Format            = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElem.InputSlotClass    = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

    // ── blend state (no blending) ───────────────────────────────────────────
    D3D12_BLEND_DESC blend = {};
    blend.AlphaToCoverageEnable  = FALSE;
    blend.IndependentBlendEnable = FALSE;
    blend.RenderTarget[0].BlendEnable           = FALSE;
    blend.RenderTarget[0].LogicOpEnable         = FALSE;
    blend.RenderTarget[0].SrcBlend              = D3D12_BLEND_ONE;
    blend.RenderTarget[0].DestBlend             = D3D12_BLEND_ZERO;
    blend.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_ZERO;
    blend.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].LogicOp               = D3D12_LOGIC_OP_NOOP;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // ── rasterizer (no cull, solid fill) ────────────────────────────────────
    D3D12_RASTERIZER_DESC raster = {};
    raster.FillMode              = D3D12_FILL_MODE_SOLID;
    raster.CullMode              = D3D12_CULL_MODE_NONE;
    raster.FrontCounterClockwise = FALSE;
    raster.DepthClipEnable       = TRUE;

    // ── depth-stencil (disabled) ────────────────────────────────────────────
    D3D12_DEPTH_STENCIL_DESC ds = {};
    ds.DepthEnable      = FALSE;
    ds.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ZERO;
    ds.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
    ds.StencilEnable    = FALSE;

    // ── PSO ─────────────────────────────────────────────────────────────────
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature        = g_rootSig.Get();
    psoDesc.VS                    = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
    psoDesc.PS                    = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
    psoDesc.BlendState            = blend;
    psoDesc.SampleMask            = UINT_MAX;
    psoDesc.RasterizerState       = raster;
    psoDesc.DepthStencilState     = ds;
    psoDesc.InputLayout           = {&inputElem, 1};
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count      = 1;

    if (FAILED(g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(g_pso.GetAddressOf()))))
        fatal("CreateGraphicsPipelineState failed");
}

void init_vertex_buffer() {
    float vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
    };
    UINT byteSize = sizeof(vertices);

    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    heap.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC res = {};
    res.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    res.Width              = byteSize;
    res.Height             = 1;
    res.DepthOrArraySize   = 1;
    res.MipLevels          = 1;
    res.Format             = DXGI_FORMAT_UNKNOWN;
    res.SampleDesc.Count   = 1;
    res.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(g_device->CreateCommittedResource(
            &heap, D3D12_HEAP_FLAG_NONE, &res,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(g_vertexBuffer.GetAddressOf()))))
        fatal("CreateCommittedResource (VB) failed");

    void* mapped = nullptr;
    g_vertexBuffer->Map(0, nullptr, &mapped);
    memcpy(mapped, vertices, byteSize);
    g_vertexBuffer->Unmap(0, nullptr);

    g_vbView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
    g_vbView.StrideInBytes  = sizeof(float) * 3;
    g_vbView.SizeInBytes    = byteSize;
}

void init_command_list() {
    if (FAILED(g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(g_cmdAlloc.GetAddressOf()))))
        fatal("CreateCommandAllocator failed");
    if (FAILED(g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            g_cmdAlloc.Get(), nullptr, IID_PPV_ARGS(g_cmdList.GetAddressOf()))))
        fatal("CreateCommandList failed");
    g_cmdList->Close();
}

} // namespace

// ─── main ─────────────────────────────────────────────────────────────────────
int main() {
    HINSTANCE hInst = GetModuleHandleW(nullptr);
    init_window(hInst);
    init_d3d12();
    init_pipeline();
    init_vertex_buffer();
    init_command_list();

    auto animStart = std::chrono::steady_clock::now();

    while (!g_closing) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                g_closing = true;
                break;
            }
        }
        if (g_closing) break;

        wait_for_gpu();

        UINT backIdx = g_swapchain->GetCurrentBackBufferIndex();
        g_cmdAlloc->Reset();
        g_cmdList->Reset(g_cmdAlloc.Get(), nullptr);

        // Barrier: PRESENT → RENDER_TARGET
        D3D12_RESOURCE_BARRIER toRT = {};
        toRT.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toRT.Transition.pResource   = g_renderTargets[backIdx].Get();
        toRT.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        toRT.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        toRT.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        g_cmdList->ResourceBarrier(1, &toRT);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvH = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvH.ptr += backIdx * g_rtvSize;
        float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
        g_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

        D3D12_VIEWPORT vp = {0.0f, 0.0f, (float)g_width, (float)g_height, 0.0f, 1.0f};
        D3D12_RECT scissor = {0, 0, (LONG)g_width, (LONG)g_height};
        g_cmdList->RSSetViewports(1, &vp);
        g_cmdList->RSSetScissorRects(1, &scissor);

        g_cmdList->OMSetRenderTargets(1, &rtvH, FALSE, nullptr);
        g_cmdList->SetGraphicsRootSignature(g_rootSig.Get());

        float t = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - animStart).count();
        g_cmdList->SetGraphicsRoot32BitConstant(0, *(UINT*)&t, 0);

        g_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_cmdList->IASetVertexBuffers(0, 1, &g_vbView);
        g_cmdList->SetPipelineState(g_pso.Get());
        g_cmdList->DrawInstanced(3, 1, 0, 0);

        // Barrier: RENDER_TARGET → PRESENT
        D3D12_RESOURCE_BARRIER toPresent = {};
        toPresent.Type  = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toPresent.Transition.pResource   = g_renderTargets[backIdx].Get();
        toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        toPresent.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
        g_cmdList->ResourceBarrier(1, &toPresent);

        g_cmdList->Close();

        ID3D12CommandList* lists[] = {g_cmdList.Get()};
        g_cmdQueue->ExecuteCommandLists(1, lists);
        g_swapchain->Present(1, 0);

        ++g_fenceVal;
        g_cmdQueue->Signal(g_fence.Get(), g_fenceVal);
    }

    // ─── cleanup ──────────────────────────────────────────────────────────────
    wait_for_gpu();

    g_vertexBuffer.Reset();
    g_pso.Reset();
    g_rootSig.Reset();
    g_cmdList.Reset();
    g_cmdAlloc.Reset();
    g_fence.Reset();
    if (g_fenceEvt) CloseHandle(g_fenceEvt);
    destroy_swapchain_dependent();
    g_rtvHeap.Reset();
    g_swapchain.Reset();
    g_cmdQueue.Reset();
    g_device.Reset();

    DestroyWindow(g_hwnd);
    UnregisterClassW(kWndClass, hInst);
    return EXIT_SUCCESS;
}
