// win_icon.cpp — isolated Win32 translation unit for setting the window icon.
// Kept separate from main.cpp so <windows.h> and <raylib.h> never share a TU
// (they have conflicting declarations for ShowCursor, CloseWindow, etc.).
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI        // avoid GDI redefinitions that clash with raylib
#define NOUSER       // only needed headers are pulled below
#undef  NOUSER       // actually we do need WM_SETICON / SendMessageW
#include <windows.h>

void SetWindowIconFromResource(void* nativeHwnd) {
    HWND hwnd = (HWND)nativeHwnd;
    if (!hwnd) return;
    // Load both large and small variants; Windows picks the best size from
    // the multi-resolution ICO (resource ID 1 from sz_resources.rc).
    HICON hLarge = (HICON)LoadImageW(
        GetModuleHandleW(NULL), MAKEINTRESOURCEW(1),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),   GetSystemMetrics(SM_CYICON),
        LR_DEFAULTCOLOR);
    HICON hSmall = (HICON)LoadImageW(
        GetModuleHandleW(NULL), MAKEINTRESOURCEW(1),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);
    if (hLarge) SendMessageW(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hLarge);
    if (hSmall) SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hSmall);
}
#else
void SetWindowIconFromResource(void*) {}
#endif
