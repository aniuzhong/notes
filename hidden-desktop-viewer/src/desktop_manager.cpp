#include "desktop_manager.h"

#include <dwmapi.h>
#include <cwctype>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")

static const std::vector<std::wstring> kIgnoredClassNames = {
    L"Progman",
    L"Button",
};

// ---- desktop enumeration callback ----
static BOOL CALLBACK EnumDesktopsCB(LPWSTR lpszDesktop, LPARAM lParam) {
    auto* out = reinterpret_cast<std::vector<DesktopInfo>*>(lParam);
    if (!out) return TRUE;
    HDESK hdesk = ::OpenDesktopW(
        lpszDesktop, 0, FALSE,
        DESKTOP_READOBJECTS  | DESKTOP_WRITEOBJECTS |
        DESKTOP_ENUMERATE    | DESKTOP_CREATEWINDOW  |
        DESKTOP_SWITCHDESKTOP);
    if (hdesk) {
        DesktopInfo info;
        info.handle = hdesk;
        info.name   = lpszDesktop;
        out->push_back(info);
    }
    return TRUE;
}

std::vector<DesktopInfo> DesktopManager::EnumerateDesktops() {
    std::vector<DesktopInfo> desktops;
    ::EnumDesktopsW(::GetProcessWindowStation(), EnumDesktopsCB,
                    reinterpret_cast<LPARAM>(&desktops));
    return desktops;
}

// ---- window enumeration callback ----
static BOOL CALLBACK EnumWindowsCB(HWND hwnd, LPARAM lParam) {
    auto* out = reinterpret_cast<std::vector<WindowInfo>*>(lParam);
    if (!out) return TRUE;

    WindowInfo info;
    info.handle = hwnd;

    int len = ::GetWindowTextLengthW(hwnd);
    if (len > 0) {
        std::vector<wchar_t> buf(len + 1);
        ::GetWindowTextW(hwnd, buf.data(), static_cast<int>(buf.size()));
        info.title = buf.data();
    }

    wchar_t cls[256]{};
    if (::GetClassNameW(hwnd, cls, _countof(cls)))
        info.className = cls;

    ::GetWindowRect(hwnd, &info.rect);
    info.visible = ::IsWindowVisible(hwnd) != 0;

    DWORD cloaked = 0;
    if (SUCCEEDED(::DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))))
        info.cloaked = (cloaked != 0);

    out->push_back(info);
    return TRUE;
}

std::vector<WindowInfo> DesktopManager::EnumerateWindows(HDESK hdesk) {
    std::vector<WindowInfo> windows;
    ::EnumDesktopWindows(hdesk, EnumWindowsCB, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

static bool IsClassNameIgnored(const std::wstring& name) {
    if (name.empty()) return true;
    for (const auto& ignored : kIgnoredClassNames) {
        if (name.size() != ignored.size()) continue;
        bool eq = true;
        for (size_t i = 0; i < name.size(); ++i) {
            if (std::towlower(name[i]) != std::towlower(ignored[i])) { eq = false; break; }
        }
        if (eq) return true;
    }
    return false;
}

std::vector<WindowInfo> DesktopManager::EnumerateVisibleWindows(HDESK hdesk) {
    auto all = EnumerateWindows(hdesk);
    std::vector<WindowInfo> result;
    result.reserve(all.size());
    for (auto& w : all) {
        if (!w.visible || w.cloaked) continue;
        if (w.title.empty()) continue;
        if (IsClassNameIgnored(w.className)) continue;
        int width  = w.rect.right  - w.rect.left;
        int height = w.rect.bottom - w.rect.top;
        if (width <= 0 || height <= 0) continue;
        result.push_back(std::move(w));
    }
    return result;
}

// ---- desktop creation / close ----
HDESK DesktopManager::CreateNewDesktop(const std::wstring& name) {
    return ::CreateDesktopW(name.c_str(), nullptr, nullptr, 0, GENERIC_ALL, nullptr);
}

void DesktopManager::CloseDesktopHandle(HDESK hdesk) {
    if (hdesk) ::CloseDesktop(hdesk);
}

// ---- name helpers ----
std::wstring DesktopManager::GetWindowStationName() {
    return GetUserObjectName(::GetProcessWindowStation());
}

std::wstring DesktopManager::GetDesktopName(HDESK hdesk) {
    return GetUserObjectName(hdesk);
}

std::wstring DesktopManager::GetUserObjectName(HANDLE obj) {
    DWORD length = 0;
    ::GetUserObjectInformationW(obj, UOI_NAME, nullptr, 0, &length);
    if (length == 0) return {};
    std::wstring name(length / sizeof(wchar_t) - 1, L'\0');
    ::GetUserObjectInformationW(obj, UOI_NAME, &name[0], length, &length);
    return name;
}

// ---- encoding helpers ----
std::string DesktopManager::ToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int sz = ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (sz <= 0) return {};
    std::string out(sz - 1, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &out[0], sz, nullptr, nullptr);
    return out;
}

std::wstring DesktopManager::FromUTF8(const std::string& str) {
    if (str.empty()) return {};
    int sz = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (sz <= 0) return {};
    std::wstring out(sz - 1, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &out[0], sz);
    return out;
}
