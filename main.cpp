#include <windows.h>
#include <windowsx.h>
#include <string>
#include <cmath>
#include <vector>
#include <shellapi.h>

#include "AppLauncher.h"
#include "RadialConfig.h"

RadialProfile activeProfile =
{
    "Default",

    8,      // optionCount
    400,    // launcherSize
    0.20,   // centerDeadZone
    0.40,   // innerRegionSize
    0.70,   // itemDistance

    { "C:\\Users\\tanma\\Desktop\\Notepad.exe - Shortcut.lnk" }
};

bool trackingMouse = false;

POINT clickPosition = {};
int selectedIndex = -1;

HHOOK mouseHook = nullptr;
HWND mainWindow = nullptr;

NOTIFYICONDATAA trayIcon = {};
#define WM_TRAYICON (WM_APP + 10)
#define ID_TRAY_CUSTOMIZE 1001
#define ID_TRAY_EXIT 1002

int GetSelectedIndex(int dx, int dy)
{
    double distance = std::sqrt(
        static_cast<double>(dx * dx + dy * dy)
    );

    const double noSelectionRadius =
        (activeProfile.launcherSize * activeProfile.centerDeadZone) / 2.0;

    if (distance < noSelectionRadius)
        return -1;

    const double PI = 3.14159265358979323846;

    double angle = std::atan2(
        static_cast<double>(dx),
        static_cast<double>(-dy)
    );

    if (angle < 0)
        angle += 2.0 * PI;

    const double angleStep =
        (2.0 * PI) / activeProfile.optionCount;

    int index = static_cast<int>(
        (angle + angleStep / 2.0) / angleStep
        );

    index %= activeProfile.optionCount;

    return index;
}

void DrawSegment(
    HDC hdc,
    int centerX,
    int centerY,
    double startAngle,
    double endAngle,
    double innerRadius,
    double outerRadius,
    bool filled
)
{
    const int pointCount = 100;
    std::vector<POINT> points;

    // Outer arc
    for (int i = 0; i <= pointCount; i++)
    {
        double t =
            startAngle +
            (endAngle - startAngle) *
            (static_cast<double>(i) / pointCount);

        POINT point;
        point.x = centerX +
            static_cast<LONG>(std::cos(t) * outerRadius);

        point.y = centerY +
            static_cast<LONG>(std::sin(t) * outerRadius);

        points.push_back(point);
    }

    // Inner arc, in reverse
    for (int i = pointCount; i >= 0; i--)
    {
        double t =
            startAngle +
            (endAngle - startAngle) *
            (static_cast<double>(i) / pointCount);

        POINT point;
        point.x = centerX +
            static_cast<LONG>(std::cos(t) * innerRadius);

        point.y = centerY +
            static_cast<LONG>(std::sin(t) * innerRadius);

        points.push_back(point);
    }

    if (filled)
    {
        HBRUSH brush =
            CreateSolidBrush(RGB(50, 120, 255));

        HBRUSH oldBrush =
            (HBRUSH)SelectObject(hdc, brush);

        Polygon(
            hdc,
            points.data(),
            static_cast<int>(points.size())
        );

        SelectObject(hdc, oldBrush);
        DeleteObject(brush);
    }
    else
    {
        HBRUSH oldBrush =
            (HBRUSH)SelectObject(
                hdc,
                GetStockObject(NULL_BRUSH)
            );

        Polygon(
            hdc,
            points.data(),
            static_cast<int>(points.size())
        );

        SelectObject(hdc, oldBrush);
    }
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
            selectedIndex = -1;

            SetWindowPos(
                mainWindow,
                HWND_TOPMOST,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW 
            );

            ShowWindow(mainWindow, SW_SHOW);
            SetForegroundWindow(mainWindow);
            BringWindowToTop(mainWindow);

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

            selectedIndex = GetSelectedIndex(dx, dy);

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

            if (selectedIndex != -1)
            {
                LaunchSelectedApp(selectedIndex);
            }

            ShowWindow(
                mainWindow,
                SW_HIDE
            );

            selectedIndex = -1;
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
                ID_TRAY_CUSTOMIZE,
                "Customize"
            );

            AppendMenuA(
                menu,
                MF_SEPARATOR,
                0,
                nullptr
            );

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
        if (LOWORD(wParam) == ID_TRAY_CUSTOMIZE)
        {
            MessageBoxA(
                hwnd,
                "Customization menu coming next.",
                "Radial Launcher",
                MB_OK
            );

            return 0;
        }

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

        const int innerSize =
            static_cast<int>(activeProfile.launcherSize * activeProfile.innerRegionSize);

        const double outerRadius =
            (activeProfile.launcherSize * 1.5) / 2.0;

        const double innerRadius =
            (innerSize * 0.5)/ 2.0;

        const double angleStep =
            (2.0 * PI) / activeProfile.optionCount;

        std::vector<std::string> labels;

        for (int i = 0; i < activeProfile.optionCount; i++)
        {
            labels.push_back(
                std::to_string(i + 1)
            );
        }

        for (int i = 0; i < activeProfile.optionCount; i++)
        {
            double centerAngle =
                (-PI / 2.0) +
                (i * angleStep);

            double startAngle =
                centerAngle - (angleStep / 2.0);

            double endAngle =
                centerAngle + (angleStep / 2.0);

            bool selected =
                (i == selectedIndex);

            DrawSegment(
                memDC,
                centerX,
                centerY,
                startAngle,
                endAngle,
                innerRadius,
                outerRadius,
                selected
            );

            double labelRadius =
                (activeProfile.launcherSize / 2.0) * activeProfile.itemDistance;

            int labelX =
                centerX +
                static_cast<int>(
                    std::cos(centerAngle) * labelRadius
                    );

            int labelY =
                centerY +
                static_cast<int>(
                    std::sin(centerAngle) * labelRadius
                    );

            DrawTextCentered(
                memDC,
                labels[i],
                labelX,
                labelY
            );
        }

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
        activeProfile.launcherSize,
        activeProfile.launcherSize,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (hwnd == nullptr)
    {
        return 0;
    }

    const int innerSize =
        static_cast<int>(activeProfile.launcherSize * activeProfile.innerRegionSize);

    const int innerOffset =
        (activeProfile.launcherSize - innerSize) / 2;

    const double outerRadius =
        activeProfile.launcherSize / 2.0;

    const double innerRadius =
        innerSize / 2.0;

    HRGN outerRegion = CreateEllipticRgn(
        0,
        0,
        activeProfile.launcherSize,
        activeProfile.launcherSize
    );

    HRGN innerRegion = CreateEllipticRgn(
        innerOffset,
        innerOffset,
        innerOffset + innerSize,
        innerOffset + innerSize
    );

    CombineRgn(
        outerRegion,
        outerRegion,
        innerRegion,
        RGN_DIFF
    );

    SetWindowRgn(
        hwnd,
        outerRegion,
        TRUE
    );

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

    int windowWidth = activeProfile.launcherSize;
    int windowHeight = activeProfile.launcherSize;

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