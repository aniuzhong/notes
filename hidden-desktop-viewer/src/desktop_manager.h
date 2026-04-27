#pragma once

#include <windows.h>

#include <string>
#include <vector>

struct DesktopInfo {
    HDESK       handle = nullptr;
    std::wstring name;
};

struct WindowInfo {
    HWND         handle = nullptr;
    std::wstring title;
    std::wstring className;
    RECT         rect{};
    bool         visible = false;
    bool         cloaked  = false;
};

class DesktopManager {
public:
    static std::vector<DesktopInfo> EnumerateDesktops();
    static std::vector<WindowInfo>  EnumerateWindows(HDESK hdesk);
    static std::vector<WindowInfo>  EnumerateVisibleWindows(HDESK hdesk);

    static HDESK       CreateNewDesktop(const std::wstring& name);
    static void        CloseDesktopHandle(HDESK hdesk);

    static std::wstring GetWindowStationName();
    static std::wstring GetDesktopName(HDESK hdesk);
    static std::wstring GetUserObjectName(HANDLE obj);

    static std::string  ToUTF8(const std::wstring& wstr);
    static std::wstring FromUTF8(const std::string& str);
};
