#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windowsx.h>
#include <commdlg.h>
#include <Shlwapi.h>

#pragma comment(lib, "ShLwApi.Lib")
#pragma comment(lib, "ComDlg32.Lib")

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
constexpr const wchar_t* kWindowTitle = L"gdi-stretchblt-artifacts";
constexpr const wchar_t* kWindowClassName = L"GDISTRETCHBLTARTIFACTS";

constexpr int kDefaultClientWidth = 960;
constexpr int kDefaultClientHeight = 540;

constexpr int kOpenButtonWidth = 140;
constexpr int kOpenButtonHeight = 36;
constexpr int kOpenButtonId = 1001;

constexpr int kStretchModeNone = -1;
constexpr UINT kMenuStretchNoneId = 2000;
constexpr UINT kMenuStretchColorOnColorId = 2001;
constexpr UINT kMenuStretchHalftoneId = 2002;

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
HBITMAP gBitmap = nullptr;
wchar_t gImagePath[MAX_PATH] = { 0 };
wchar_t gOpenErrorText[MAX_PATH * 2] = { 0 };
HWND gOpenButton = nullptr;
bool gHasOpenError = false;
int gStretchBltMode = HALFTONE;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, int clientWidth, int clientHeight);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void ResizeWindowToClient(HWND hWnd, int clientWidth, int clientHeight);
void UpdateOpenButtonLayout(HWND hWnd);
bool OpenBitmapFromDialog(HWND hWnd);
void ShowStretchModeContextMenu(HWND hWnd, int screenX, int screenY);

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow, kDefaultClientWidth, kDefaultClientHeight))
    {
        return FALSE;
    }

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// ---------------------------------------------------------------------------
// Window/class initialization
// ---------------------------------------------------------------------------
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = kWindowClassName;
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, int clientWidth, int clientHeight)
{
    HWND hWnd = CreateWindowW(
        kWindowClassName,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        0,
        0,
        0,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ResizeWindowToClient(hWnd, clientWidth, clientHeight);
    SetWindowTextW(hWnd, kWindowTitle);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void ResizeWindowToClient(HWND hWnd, int clientWidth, int clientHeight)
{
    RECT rc = { 0, 0, clientWidth, clientHeight };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    const int windowWidth = rc.right - rc.left;
    const int windowHeight = rc.bottom - rc.top;

    SetWindowPos(hWnd, nullptr, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);
}

void UpdateOpenButtonLayout(HWND hWnd)
{
    if (!gOpenButton)
    {
        return;
    }

    if (gBitmap)
    {
        ShowWindow(gOpenButton, SW_HIDE);
        return;
    }

    ShowWindow(gOpenButton, SW_SHOW);

    RECT clientRect = {};
    GetClientRect(hWnd, &clientRect);

    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    const int x = (clientWidth - kOpenButtonWidth) / 2;
    int y = (clientHeight - kOpenButtonHeight) / 2;
    if (gHasOpenError)
    {
        y += 30;
    }

    MoveWindow(gOpenButton, x, y, kOpenButtonWidth, kOpenButtonHeight, TRUE);
}

bool OpenBitmapFromDialog(HWND hWnd)
{
    OPENFILENAMEW ofn = {};
    wchar_t selectedPath[MAX_PATH] = L"";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = selectedPath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Bitmap Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameW(&ofn))
    {
        return false;
    }

    HBITMAP loadedBitmap = (HBITMAP)LoadImageW(
        nullptr,
        selectedPath,
        IMAGE_BITMAP,
        0,
        0,
        LR_LOADFROMFILE | LR_CREATEDIBSECTION);

    if (!loadedBitmap)
    {
        lstrcpynW(gImagePath, selectedPath, MAX_PATH);
        lstrcpynW(gOpenErrorText, L"Cannot open ", MAX_PATH * 2);
        lstrcatW(gOpenErrorText, gImagePath);
        gHasOpenError = true;
        return false;
    }

    if (gBitmap)
    {
        DeleteObject(gBitmap);
    }

    gBitmap = loadedBitmap;
    gHasOpenError = false;
    gOpenErrorText[0] = L'\0';
    lstrcpynW(gImagePath, selectedPath, MAX_PATH);

    BITMAP bmp = {};
    if (GetObject(gBitmap, sizeof(BITMAP), &bmp) == sizeof(BITMAP))
    {
        ResizeWindowToClient(hWnd, bmp.bmWidth, bmp.bmHeight);
    }

    return true;
}

void ShowStretchModeContextMenu(HWND hWnd, int screenX, int screenY)
{
    HMENU popupMenu = CreatePopupMenu();
    if (!popupMenu)
    {
        return;
    }

    AppendMenuW(popupMenu, MF_STRING, kMenuStretchNoneId, L"NONE");
    AppendMenuW(popupMenu, MF_STRING, kMenuStretchColorOnColorId, L"COLORONCOLOR");
    AppendMenuW(popupMenu, MF_STRING, kMenuStretchHalftoneId, L"HALFTONE");

    const UINT checkedMenuId =
        (gStretchBltMode == kStretchModeNone)
            ? kMenuStretchNoneId
            : ((gStretchBltMode == COLORONCOLOR) ? kMenuStretchColorOnColorId : kMenuStretchHalftoneId);
    CheckMenuRadioItem(
        popupMenu,
        kMenuStretchNoneId,
        kMenuStretchHalftoneId,
        checkedMenuId,
        MF_BYCOMMAND);

    if (screenX == -1 && screenY == -1)
    {
        RECT clientRect = {};
        GetClientRect(hWnd, &clientRect);
        POINT center = {
            (clientRect.right - clientRect.left) / 2,
            (clientRect.bottom - clientRect.top) / 2
        };
        ClientToScreen(hWnd, &center);
        screenX = center.x;
        screenY = center.y;
    }

    const UINT command = TrackPopupMenu(
        popupMenu,
        TPM_RIGHTBUTTON | TPM_RETURNCMD,
        screenX,
        screenY,
        0,
        hWnd,
        nullptr);

    if (command == kMenuStretchNoneId)
    {
        gStretchBltMode = kStretchModeNone;
        InvalidateRect(hWnd, nullptr, TRUE);
    }
    else if (command == kMenuStretchColorOnColorId)
    {
        gStretchBltMode = COLORONCOLOR;
        InvalidateRect(hWnd, nullptr, TRUE);
    }
    else if (command == kMenuStretchHalftoneId)
    {
        gStretchBltMode = HALFTONE;
        InvalidateRect(hWnd, nullptr, TRUE);
    }

    DestroyMenu(popupMenu);
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        gOpenButton = CreateWindowW(
            L"BUTTON",
            L"Open bitmap",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            0,
            0,
            kOpenButtonWidth,
            kOpenButtonHeight,
            hWnd,
            (HMENU)(INT_PTR)kOpenButtonId,
            (HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE),
            nullptr);
        UpdateOpenButtonLayout(hWnd);
    }
    break;

    case WM_COMMAND:
        if (LOWORD(wParam) == kOpenButtonId && HIWORD(wParam) == BN_CLICKED)
        {
            OpenBitmapFromDialog(hWnd);
            UpdateOpenButtonLayout(hWnd);
            InvalidateRect(hWnd, nullptr, TRUE);
            return 0;
        }
        break;

    case WM_CONTEXTMENU:
        ShowStretchModeContextMenu(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps = {};
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT clientRect = {};
        GetClientRect(hWnd, &clientRect);

        if (gBitmap)
        {
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, gBitmap);

            BITMAP bmp = {};
            GetObject(gBitmap, sizeof(BITMAP), &bmp);

            if (gStretchBltMode != kStretchModeNone)
            {
                SetStretchBltMode(hdc, gStretchBltMode);
                if (gStretchBltMode == HALFTONE)
                {
                    SetBrushOrgEx(hdc, 0, 0, nullptr);
                }
            }

            StretchBlt(
                hdc,
                0,
                0,
                clientRect.right - clientRect.left,
                clientRect.bottom - clientRect.top,
                hdcMem,
                0,
                0,
                bmp.bmWidth,
                bmp.bmHeight,
                SRCCOPY);

            SelectObject(hdcMem, hOldBmp);
            DeleteDC(hdcMem);
        }
        else if (gHasOpenError)
        {
            SetBkMode(hdc, TRANSPARENT);
            DrawTextW(
                hdc,
                gOpenErrorText,
                -1,
                &clientRect,
                DT_CENTER | DT_VCENTER | DT_WORDBREAK);
        }

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_SIZE:
        UpdateOpenButtonLayout(hWnd);
        InvalidateRect(hWnd, nullptr, TRUE);
        break;

    case WM_DESTROY:
        if (gBitmap)
        {
            DeleteObject(gBitmap);
        }
        PostQuitMessage(0);
        break;

    default:
        UNREFERENCED_PARAMETER(wParam);
        UNREFERENCED_PARAMETER(lParam);
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}
