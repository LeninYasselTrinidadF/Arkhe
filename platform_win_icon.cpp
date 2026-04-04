#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <cstdio>

// Don't include raylib.h here to avoid symbol collisions between raylib and
// Windows headers (e.g. CloseWindow/ShowCursor). Declare the minimal
// raylib symbol we need.
extern "C" void* GetWindowHandle(void);

// Set the Win32 window/class icon from the embedded resource with ID 1.
void set_win32_icon_from_resource() {
    printf("set_win32_icon_from_resource(): starting...\n");

    HMODULE hModule = GetModuleHandleA(NULL);
    printf("GetModuleHandleA(NULL) -> %p\n", hModule);
    if (!hModule) {
        printf("Failed to get module handle.\n");
        return;
    }

    HICON hIcon = (HICON)LoadIconA(hModule, MAKEINTRESOURCEA(1));
    printf("LoadIconA(hModule, 1) -> %p\n", hIcon);

    if (!hIcon) {
        printf("LoadIcon failed, trying LoadImage...\n");
        hIcon = (HICON)LoadImageA(hModule, MAKEINTRESOURCEA(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
        printf("LoadImageA(hModule, 1) -> %p\n", hIcon);
    }

    if (!hIcon) {
        printf("Failed to load icon from resource (ID 1).\n");
        return;
    }

    void* handle = GetWindowHandle();
    HWND hwnd = (HWND)handle;
    printf("GetWindowHandle() -> %p (HWND)\n", hwnd);

    if (!hwnd) {
        printf("Failed to get HWND from raylib.\n");
        return;
    }

    printf("Sending WM_SETICON messages and SetClassLongPtr...\n");
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)hIcon);
    SetClassLongPtr(hwnd, GCLP_HICONSM, (LONG_PTR)hIcon);

    // Force Windows to redraw the window and update cached icon info.
    // This is especially important for the taskbar icon.
    printf("Forcing window redraw and taskbar update...\n");
    InvalidateRect(hwnd, NULL, FALSE);
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

    // Notify the shell (Explorer) that window properties have changed.
    // This forces taskbar to update.
    SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 
                       (LPARAM)"ShellIconSize", SMTO_ABORTIFHUNG, 100, NULL);

    printf("set_win32_icon_from_resource(): complete.\n");
}
#endif
