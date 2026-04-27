#include "capture_manager.h"

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

CaptureManager::~CaptureManager() { Stop(); }

bool CaptureManager::StartDesktopCapture(HDESK hdesk) {
    if (running_.load()) return false;
    stopRequested_.store(false);
    thread_ = std::thread(&CaptureManager::DesktopCaptureThread, this, hdesk);
    running_.store(true);
    return true;
}

bool CaptureManager::StartWindowCapture(HDESK hdesk, HWND hwnd) {
    if (running_.load()) return false;
    if (!hwnd || !::IsWindow(hwnd)) return false;
    stopRequested_.store(false);
    thread_ = std::thread(&CaptureManager::WindowCaptureThread, this, hdesk, hwnd);
    running_.store(true);
    return true;
}

void CaptureManager::Stop() {
    if (!running_.load()) return;
    stopRequested_.store(true);
    if (thread_.joinable()) thread_.join();
    running_.store(false);
}

bool CaptureManager::GetLatestFrame(CapturedFrame& outFrame) {
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (!latestFrame_.valid) return false;
    outFrame = latestFrame_;
    latestFrame_.valid = false;
    return true;
}

// ---------------------------------------------------------------------------
// Desktop capture: composite all visible windows with Z-order (GDI)
// ---------------------------------------------------------------------------
void CaptureManager::DesktopCaptureThread(HDESK hdesk) {
    if (hdesk) ::SetThreadDesktop(hdesk);

    HDC screenDC = ::GetDC(nullptr);
    if (!screenDC) return;

    const int dw = ::GetSystemMetrics(SM_CXSCREEN);
    const int dh = ::GetSystemMetrics(SM_CYSCREEN);

    BITMAPINFO bi{};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = dw;
    bi.bmiHeader.biHeight      = -dh;   // top-down
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC     capDC  = ::CreateCompatibleDC(screenDC);
    HBITMAP capBmp = ::CreateCompatibleBitmap(screenDC, dw, dh);
    HGDIOBJ oldBmp = ::SelectObject(capDC, capBmp);

    const UINT kRenderFlag = PW_RENDERFULLCONTENT;

    while (!stopRequested_.load()) {
        RECT dr;
        ::GetClientRect(::GetDesktopWindow(), &dr);
        ::FillRect(capDC, &dr, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));

        // Collect windows + Z-order
        std::vector<std::pair<HWND, int>> windowList;
        auto enumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
            auto* list = reinterpret_cast<std::vector<std::pair<HWND, int>>*>(lParam);
            HWND prev = ::GetWindow(hwnd, GW_HWNDPREV);
            int z = 0;
            while (prev) { ++z; prev = ::GetWindow(prev, GW_HWNDPREV); }
            list->push_back({ hwnd, z });
            return TRUE;
        };
        ::EnumWindows(enumProc, reinterpret_cast<LPARAM>(&windowList));

        std::stable_sort(windowList.begin(), windowList.end(),
            [](auto& a, auto& b) { return a.second > b.second; });

        for (auto& [wnd, z] : windowList) {
            if (!::IsWindow(wnd) || !::IsWindowVisible(wnd)) continue;
            RECT wr;
            if (!::GetWindowRect(wnd, &wr)) continue;
            int w = wr.right - wr.left, h = wr.bottom - wr.top;
            if (w <= 0 || h <= 0) continue;

            RECT dcr;
            ::GetClientRect(::GetDesktopWindow(), &dcr);
            int dx = wr.left - dcr.left, dy = wr.top - dcr.top;

            HDC     wdc    = ::GetWindowDC(wnd);
            HDC     wCapDC = ::CreateCompatibleDC(screenDC);
            HBITMAP wBmp   = ::CreateCompatibleBitmap(screenDC, w, h);
            HGDIOBJ wOld   = ::SelectObject(wCapDC, wBmp);

            if (!::PrintWindow(wnd, wCapDC, kRenderFlag)) {
                // fallback silently
            }
            ::BitBlt(capDC, dx, dy, w, h, wCapDC, 0, 0, SRCCOPY);

            ::SelectObject(wCapDC, wOld);
            ::DeleteObject(wBmp);
            ::DeleteDC(wCapDC);
            ::ReleaseDC(wnd, wdc);
        }

        // Read pixels
        std::vector<uint8_t> pixels(static_cast<size_t>(dw) * dh * 4);
        if (::GetDIBits(capDC, capBmp, 0, dh, pixels.data(), &bi, DIB_RGB_COLORS)) {
            std::lock_guard<std::mutex> lock(frameMutex_);
            latestFrame_.pixels = std::move(pixels);
            latestFrame_.width  = dw;
            latestFrame_.height = dh;
            latestFrame_.valid  = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    ::SelectObject(capDC, oldBmp);
    ::DeleteObject(capBmp);
    ::DeleteDC(capDC);
    ::ReleaseDC(nullptr, screenDC);
}

// ---------------------------------------------------------------------------
// Window capture: single window via PrintWindow (GDI)
// ---------------------------------------------------------------------------
void CaptureManager::WindowCaptureThread(HDESK hdesk, HWND hwnd) {
    if (hdesk) ::SetThreadDesktop(hdesk);

    HDC screenDC = ::GetDC(nullptr);
    if (!screenDC) return;

    const UINT kRenderFlag = PW_RENDERFULLCONTENT;

    while (!stopRequested_.load()) {
        if (!::IsWindow(hwnd) || !::IsWindowVisible(hwnd)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        RECT wr;
        if (!::GetWindowRect(hwnd, &wr)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        int w = wr.right - wr.left, h = wr.bottom - wr.top;
        if (w <= 0 || h <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        HDC     wdc    = ::GetWindowDC(hwnd);
        HDC     memDC  = ::CreateCompatibleDC(screenDC);
        HBITMAP bmp    = ::CreateCompatibleBitmap(screenDC, w, h);
        HGDIOBJ oldBmp = ::SelectObject(memDC, bmp);

        if (!::PrintWindow(hwnd, memDC, kRenderFlag)) {
            ::BitBlt(memDC, 0, 0, w, h, wdc, 0, 0, SRCCOPY | CAPTUREBLT);
        }

        BITMAPINFO bi{};
        bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth       = w;
        bi.bmiHeader.biHeight      = -h;
        bi.bmiHeader.biPlanes      = 1;
        bi.bmiHeader.biBitCount    = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        std::vector<uint8_t> pixels(static_cast<size_t>(w) * h * 4);
        if (::GetDIBits(memDC, bmp, 0, h, pixels.data(), &bi, DIB_RGB_COLORS)) {
            std::lock_guard<std::mutex> lock(frameMutex_);
            latestFrame_.pixels = std::move(pixels);
            latestFrame_.width  = w;
            latestFrame_.height = h;
            latestFrame_.valid  = true;
        }

        ::SelectObject(memDC, oldBmp);
        ::DeleteObject(bmp);
        ::DeleteDC(memDC);
        ::ReleaseDC(hwnd, wdc);

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    ::ReleaseDC(nullptr, screenDC);
}
