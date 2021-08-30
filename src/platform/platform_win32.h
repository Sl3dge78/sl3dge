#ifndef PLATFORM_WIN32_H
#define PLATFORM_WIN32_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct PlatformWindow {
    HWND hwnd;
    HINSTANCE hinstance;
    HDC dc;
    HGLRC opengl_rc;
    u32 w;
    u32 h;
} PlatformWindow;

#endif
