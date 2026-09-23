#include <windows.h>
#include <shellapi.h>
#include <string>

void LaunchSelectedApp(int selectedIndex)
{
    const char* appPaths[8] =
    {
        "C:\\Windows\\System32\\notepad.exe", // N
        "",                                    // NE
        "",                                    // E
        "",                                    // SE
        "",                                    // S
        "",                                    // SW
        "",                                    // W
        ""                                     // NW
    };

    if (selectedIndex < 0 || selectedIndex >= 8)
        return;

    if (appPaths[selectedIndex][0] == '\0')
        return;

    HINSTANCE result = ShellExecuteA(
        nullptr,
        "open",
        appPaths[selectedIndex],
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );

    if ((INT_PTR)result <= 32)
    {
        std::string errorMessage;

        switch ((INT_PTR)result)
        {
        case SE_ERR_FNF:
            errorMessage = "The specified file was not found.";
            break;

        case SE_ERR_PNF:
            errorMessage = "The specified path was not found.";
            break;

        case SE_ERR_ACCESSDENIED:
            errorMessage = "Access was denied.";
            break;

        case SE_ERR_OOM:
            errorMessage = "There was not enough memory.";
            break;

        case SE_ERR_SHARE:
            errorMessage = "A sharing violation occurred.";
            break;

        default:
            errorMessage =
                "Windows returned an unknown ShellExecute error.";
            break;
        }

        MessageBoxA(
            nullptr,
            errorMessage.c_str(),
            "Radial Launcher - Launch Error",
            MB_OK | MB_ICONERROR
        );
    }
}