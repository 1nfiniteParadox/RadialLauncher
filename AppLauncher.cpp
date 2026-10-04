#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>

#include "RadialConfig.h"

void LaunchSelectedApp(int selectedIndex)
{
    std::vector<std::string> appPaths(optionCount, "");

    appPaths[0] = "C:\\Users\\tanma\\Desktop\\Notepad.exe - Shortcut.lnk";

    if (selectedIndex < 0 || selectedIndex >= optionCount)
        return;

    if (appPaths[selectedIndex].empty())
        return;

    HINSTANCE result = ShellExecuteA(
        nullptr,
        "open",
        appPaths[selectedIndex].c_str(),
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