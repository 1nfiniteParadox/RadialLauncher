#include <windows.h>
#include <windowsx.h>
#include <string>
#include <cmath>
#include <vector>
#include <shellapi.h>

#include "AppLauncher.h"

bool trackingMouse = false;

POINT clickPosition = {};
std::string direction = "Click and Move";

HHOOK mouseHook = nullptr;
HWND mainWindow = nullptr;

NOTIFYICONDATAA trayIcon = {};
#define WM_TRAYICON (WM_APP + 10)
#define ID_TRAY_EXIT 1001

std::string GetDirection(int dx, int dy)
{
    double distance = std::sqrt(
        static_cast<double>(dx * dx + dy * dy)
    );

    if (distance < 30.0)
        return "CENTER";

    double angle = std::atan2(
        -dy,
        dx
    ) * 180.0 / 3.14159265358979323846;

    if (angle < 0)
        angle += 360.0;

    if (angle >= 337.5 || angle < 22.5)
        return "EAST";

    if (angle < 67.5)
        return "NORTHEAST";
    
    if (angle < 112.5)
        return "NORTH";

    if (angle < 157.5)
        return "NORTHWEST";

    if (angle < 202.5)
        return "WEST";

    if (angle < 247.5)
        return "SOUTHWEST";

    if (angle < 292.5)
        return "SOUTH";

    return "SOUTHEAST";
}

void DrawCircle(
    HDC hdc,
    int centerX,
    int centerY,
    int radius
)
{
    Ellipse(
        hdc,
        centerX - radius,
        centerY - radius,
        centerX + radius,
        centerY + radius
    );
}

void DrawFilledCircle(
    HDC hdc,
    int centerX,
    int centerY,
    int radius
)
{
    HBRUSH brush = CreateSolidBrush(RGB(50, 120, 255));

    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

    Ellipse(
        hdc,
        centerX - radius,
        centerY - radius,
        centerX + radius,
        centerY + radius
    );

    SelectObject(hdc, oldBrush);
    DeleteObject(brush);
}

void DrawTextCentered(
    HDC hdc,
    const std::string& text,
    int centerX,
    int centerY
)
{
    SIZE size;

    GetTextExtentPoint32A(
        hdc,
        text.c_str(),
        static_cast<int>(text.length()),
        &size
    );

    TextOutA(
        hdc,
        centerX - size.cx / 2,
        centerY - size.cy / 2,
        text.c_str(),
        static_cast<int>(text.length())
    );
}

int DirectionToIndex(const std::string& direction)
{
    if (direction == "NORTH")
        return 0;

    if (direction == "NORTHEAST")
        return 1;

    if (direction == "EAST")
        return 2;

    if (direction == "SOUTHEAST")
        return 3;

    if (direction == "SOUTH")
        return 4;

    if (direction == "SOUTHWEST")
        return 5;

    if (direction == "WEST")
        return 6;

    if (direction == "NORTHWEST")
        return 7;

    return -1;
}

LRESULT CALLBACK LowLevelMouseProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
)
{
    if (nCode == HC_ACTION)
    {
        MSLLHOOKSTRUCT* mouse = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);

        if (wParam == WM_MBUTTONDOWN)
        {
            clickPosition = mouse->pt;

            trackingMouse = true;
            direction = "CENTER";

            SetWindowPos(
                mainWindow,
                HWND_TOPMOST,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW 
            );

            PostMessage(
                mainWindow,
                WM_APP + 1,
                0,
                0
            );
        }
        else if (wParam == WM_MOUSEMOVE && trackingMouse)
        {
            int dx = mouse->pt.x - clickPosition.x;

            int dy = mouse->pt.y - clickPosition.y;

            direction = GetDirection(dx, dy);

            PostMessage(
                mainWindow,
                WM_APP + 1,
                0,
                0
            );
        }
        else if (wParam == WM_MBUTTONUP)
        {
            trackingMouse = false;

            int selectedIndex = DirectionToIndex(direction);

            if (selectedIndex != -1)
            {
                LaunchSelectedApp(selectedIndex);
            }

            ShowWindow(
                mainWindow,
                SW_HIDE
            );
        }
    }

    return CallNextHookEx(
        mouseHook,
        nCode,
        wParam,
        lParam
    );
}

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_APP + 1:
    {
        InvalidateRect(hwnd, nullptr, FALSE);

        return 0;
    }

    case WM_TRAYICON:
    {
        if (lParam == WM_RBUTTONUP)
        {
            HMENU menu = CreatePopupMenu();

            AppendMenuA(
                menu,
                MF_STRING,
                ID_TRAY_EXIT,
                "Exit"
            );

            POINT cursorPosition;
            GetCursorPos(&cursorPosition);

            SetForegroundWindow(hwnd);

            TrackPopupMenu(
                menu,
                TPM_RIGHTBUTTON,
                cursorPosition.x,
                cursorPosition.y,
                0,
                hwnd,
                nullptr
            );

            DestroyMenu(menu);
        }

        return 0;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_TRAY_EXIT)
        {
            Shell_NotifyIconA(
                NIM_DELETE,
                &trayIcon
            );

            DestroyWindow(hwnd);
            return 0;
        }

        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        // Create memory device context
        HDC memDC = CreateCompatibleDC(hdc);

        // Create bitmap that matches the window
        HBITMAP memBitmap = CreateCompatibleBitmap(
            hdc,
            width,
            height
        );

        // Put bitmap into the memory DC
        HBITMAP oldBitmap = (HBITMAP)SelectObject(
            memDC,
            memBitmap
        );

        //Clear the memory surface
        FillRect(
            memDC,
            &rect,
            (HBRUSH)(COLOR_WINDOW + 1)
        );

        int centerX = width / 2;
        int centerY = height / 2;

        const double PI = 3.14159265358979323846;

        int distance = 150;
        int radius = 30;

        int selectedIndex = DirectionToIndex(direction);

        const char* labels[8] =
        {
            "N",
            "NE",
            "E",
            "SE",
            "S",
            "SW",
            "W",
            "NW"
        };

        // Draw radial menu
        for (int i = 0; i < 8; i++)
        {
            double angle = (- PI / 2.0) + (i * (2.0 * PI / 8.0));

            int x = centerX + static_cast<int>(std::cos(angle) * distance);
            int y = centerY + static_cast<int>(std::sin(angle) * distance);

            bool selected = (i == selectedIndex);

            if (selected)
            {
                DrawFilledCircle(
                    memDC,
                    x,
                    y,
                    radius + 5
                );
            }
            else
            {
                DrawCircle(
                    memDC,
                    x,
                    y,
                    radius
                );
            }

            DrawTextCentered(
                memDC,
                labels[i],
                x,
                y
            );
        }

        // Draw center circle of menu
        DrawCircle(
            memDC,
            centerX,
            centerY,
            20
        );

        // Copy finished image to actual window
        BitBlt(
            hdc,
            0,
            0,
            width,
            height,
            memDC,
            0,
            0,
            SRCCOPY
        );

        // Restore old bitmap
        SelectObject(
            memDC,
            oldBitmap
        );

        // CLean up
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);

        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(
        hwnd, 
        uMsg, 
        wParam, 
        lParam
    );
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    const char CLASS_NAME[] = "RadialLauncherWindow";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = nullptr;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowExA(
        WS_EX_TOOLWINDOW,
        CLASS_NAME,
        "Radial Launcher",
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        500,
        500,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (hwnd == nullptr)
    {
        return 0;
    }

    mainWindow = hwnd;

    trayIcon.cbSize = sizeof(NOTIFYICONDATAA);
    trayIcon.hWnd = hwnd;
    trayIcon.uID = 1;
    trayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    trayIcon.uCallbackMessage = WM_TRAYICON;
    trayIcon.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    strcpy_s(
        trayIcon.szTip,
        "Radial Launcher"
    );

    Shell_NotifyIconA(
        NIM_ADD,
        &trayIcon
    );

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int windowWidth = 500;
    int windowHeight = 500;

    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    SetWindowPos(
        hwnd,
        HWND_TOP,
        x,
        y,
        windowWidth,
        windowHeight,
        SWP_SHOWWINDOW
    );

    ShowWindow(
        hwnd, 
        SW_HIDE 
    );

    mouseHook = SetWindowsHookExA(
        WH_MOUSE_LL,
        LowLevelMouseProc,
        GetModuleHandle(nullptr),
        0
    );

    MSG msg = {};

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (mouseHook)
    {
        UnhookWindowsHookEx(mouseHook);
    }

    return 0;
}