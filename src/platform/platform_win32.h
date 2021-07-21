#ifndef PLATFORM_WIN32_H
#define PLATFORM_WIN32_H

#include <windows.h>

typedef struct PlatformWindow {
    HWND hwnd;
    HINSTANCE hinstance;
} PlatformWindow;

#endif