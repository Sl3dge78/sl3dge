#ifndef PLATFORM_WIN32_H
#define PLATFORM_WIN32_H

#include <windows.h>

typedef struct PlatformWindow {
    HWND hwnd;
    HINSTANCE hinstance;
    HDC dc;
    u32 h;
    u32 w;
    HGLRC opengl_rc;
} PlatformWindow;

#endif