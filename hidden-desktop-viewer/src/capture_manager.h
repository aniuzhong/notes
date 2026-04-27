#pragma once

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

struct CapturedFrame {
    std::vector<uint8_t> pixels;   // BGRA 32-bit
    int  width  = 0;
    int  height = 0;
    bool valid  = false;
};

class CaptureManager {
public:
    CaptureManager() = default;
    ~CaptureManager();

    bool StartDesktopCapture(HDESK hdesk);
    bool StartWindowCapture(HDESK hdesk, HWND hwnd);
    void Stop();

    bool IsRunning() const { return running_.load(); }
    bool GetLatestFrame(CapturedFrame& outFrame);

private:
    void DesktopCaptureThread(HDESK hdesk);
    void WindowCaptureThread(HDESK hdesk, HWND hwnd);

    std::thread       thread_;
    std::atomic<bool> stopRequested_{ false };
    std::atomic<bool> running_{ false };
    std::mutex        frameMutex_;
    CapturedFrame     latestFrame_;
};
