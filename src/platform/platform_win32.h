#pragma once

typedef struct PlatformWindow {
    HWND hwnd;
    HINSTANCE hinstance;
    HDC dc;
    HGLRC opengl_rc;
    u32 w;
    u32 h;
} PlatformWindow;
