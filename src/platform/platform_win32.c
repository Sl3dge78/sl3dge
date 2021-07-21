// Global includes

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define __WIN32__
#include <windows.h>

#include <sl3dge-utils/sl3dge.h>

#include "platform.h"
#include "platform_win32.h"

#include "game.h"
#include "renderer/renderer.c"

typedef struct ShaderCode {
    const char *spv_path;
    FILETIME last_write_time;
} ShaderCode;

/* GLOBALS */

global HANDLE stderrHandle;
global bool mouse_captured; // TODO this probably shouldn't be a global
global Renderer *renderer;
global PlatformWindow global_window;

/* FUNCTIONS */

void Win32GameLoadFunctions(Module *dll) {
    pfn_GameStart = (GameStart_t *)GetProcAddress(dll->dll, "GameStart");
    pfn_GameLoop = (GameLoop_t *)GetProcAddress(dll->dll, "GameLoop");
}

void Win32GameLoadRendererAPI(Renderer *renderer, GameData *game_data) {
    game_data->renderer = renderer;
    game_data->renderer_api.LoadMesh = &RendererLoadMesh;
    game_data->renderer_api.DestroyMesh = &RendererDestroyMesh;
    game_data->renderer_api.InstantiateMesh = &RendererInstantiateMesh;
    game_data->renderer_api.SetCamera = &RendererSetCamera;
    game_data->renderer_api.SetSunDirection = &RendererSetSunDirection;
}

i64 PlatformGetTicks() {
    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);
    return ticks.QuadPart;
}

void PlatformSetCaptureMouse(bool val) {
    if(mouse_captured == val)
        return;
    if(val) {
        SetCapture(GetActiveWindow());
        ShowCursor(FALSE);
    } else {
        ReleaseCapture();
        ShowCursor(TRUE);
    }
    mouse_captured = val;
}

// if result is NULL, function will query the file size for allocation in file_size.
void PlatformReadBinary(const char *path, i64 *file_size, u32 *result) {
    FILE *file;
    fopen_s(&file, path, "rb");
    if(!file) {
        sError(0, "Unable to open file");
        sError(0, path);
    }
    // Get the size
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    if(!result) { // result if null, we're in query mode
        fclose(file);
        return;
    }
    rewind(file);
    // Copy into result
    fread(result, 1, *file_size, file);

    fclose(file);
}

// TODO : Handle UTF8
void Win32Log(const char *message, u8 level) {
    unsigned long charsWritten;

    static u8 levels[] = {FOREGROUND_INTENSITY,
                          FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
                          FOREGROUND_RED | FOREGROUND_GREEN,
                          FOREGROUND_RED};
    ASSERT(level < ARRAY_SIZE(levels));
    SetConsoleTextAttribute(stderrHandle, levels[level]);
    WriteConsoleA(stderrHandle, message, lstrlen(message), &charsWritten, NULL);
    OutputDebugStringA(message);
    SetConsoleTextAttribute(stderrHandle, levels[1]);
}

LRESULT CALLBACK Win32WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch(msg) {
    case WM_DESTROY: sWarn("WM_DESTROY"); return 0;
    case WM_CLOSE: PostQuitMessage(0); return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        sError("Keydown intercepted in window proc, this shouldn't happen");
        return 0;
    case WM_SIZE:
    case WM_SIZING: {
        RendererUpdateWindow(renderer, &global_window);
        return 0;
    }
    default: return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

void Win32HandleKeyboardMessages(LPARAM lparam, GameInput *input) {
    u32 scancode = (lparam >> 16) & 0x7F;
    bool is_down = !(lparam & (1 << 31));
    bool was_down = !(lparam & (1 << 30));
    u8 value = 0;
    if(is_down) {
        value |= KEY_PRESSED;
        sTrace("Keydown : 0x%x", scancode);
        if(!was_down)
            value |= KEY_DOWN;
    } else if(was_down) {
        value |= KEY_UP;
    }

    input->keyboard[scancode] = value;
}

bool Win32CreateWindow(HINSTANCE instance, PlatformWindow *window) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = Win32WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = "Vulkan";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("Vulkan",
                             "Vulkan",
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             1280,
                             720,
                             NULL,
                             NULL,
                             instance,
                             NULL);
    if(hwnd == NULL) {
        DWORD error = GetLastError();
        sError("WIN32 : Unable to create window. Error : %d", error);
        return false;
    }

    window->hinstance = instance;
    window->hwnd = hwnd;

    return true;
}

i32 WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR cmd_line, INT cmd_show) {
    AttachConsole(ATTACH_PARENT_PROCESS);
    stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
    sLogSetCallback(&Win32Log);

    if(!Win32CreateWindow(instance, &global_window)) {
        return -1;
    }

    PlatformAPI platform_api = {0};
    platform_api.ReadBinary = &PlatformReadBinary;
    platform_api.SetCaptureMouse = &PlatformSetCaptureMouse;

    renderer = RendererCreate(&global_window, &platform_api);

    Module game_module = {0};
    Win32LoadModule(&game_module, "game");
    Win32GameLoadFunctions(&game_module);

    GameData game_data = {0};
    GameInput input = {0};
    game_data.platform_api = platform_api;
    Win32GameLoadRendererAPI(renderer, &game_data);

    ShowWindow(global_window.hwnd, cmd_show);
    pfn_GameStart(&game_data);

    bool running = true;
    f32 delta_time = 0;
    i64 frame_start = PlatformGetTicks();
    sLog("Running");
    while(running) {
        {
            // Sync
            i64 time = PlatformGetTicks();
            delta_time = (float)(time - frame_start) / 10000000.f;
            frame_start = time;
        }

        RECT window_size;
        GetWindowRect(global_window.hwnd, &window_size);

        { // Hot Reloading
            if(Win32ShouldReloadModule(&game_module)) {
                Win32CloseModule(&game_module);
                Win32LoadModule(&game_module, "game");
                Win32GameLoadFunctions(&game_module);
                sLog("Game code reloaded");
            }
        }

        { // Mouse Position
            POINT pos;
            GetCursorPos(&pos);
            input.mouse_delta_x = pos.x - input.mouse_x;
            input.mouse_delta_y = pos.y - input.mouse_y;

            if(mouse_captured) {
                u32 center_x = window_size.left + window_size.right / 2;
                u32 center_y = window_size.top + window_size.bottom / 2;
                input.mouse_x = center_x;
                input.mouse_y = center_y;
                SetCursorPos(center_x, center_y);
            } else {
                input.mouse_x = pos.x;
                input.mouse_y = pos.y;
            }
        }

        MSG msg = {0};
        while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            BOOL value = GetMessage(&msg, NULL, 0, 0);
            TranslateMessage(&msg);
            if(value != 0) {
                switch(msg.message) {
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYUP: Win32HandleKeyboardMessages(msg.lParam, &input); break;
                case WM_LBUTTONDOWN: input.mouse |= MOUSE_LEFT; break;
                case WM_LBUTTONUP: input.mouse &= ~MOUSE_LEFT; break;
                case WM_MBUTTONDOWN: input.mouse |= MOUSE_MIDDLE; break;
                case WM_MBUTTONUP: input.mouse &= ~MOUSE_MIDDLE; break;
                case WM_RBUTTONDOWN: input.mouse |= MOUSE_RIGHT; break;
                case WM_RBUTTONUP: input.mouse &= ~MOUSE_RIGHT; break;
                case WM_EXITSIZEMOVE: sLog("exit!"); break;
                default:
                    //sLog("MSG: %x", msg.message);
                    DispatchMessage(&msg);
                    break;
                }
            } else if(value == 0) {
                running = false;
                break;
            } else {
                sError("WIN32 : Message error");
            }
        }

        pfn_GameLoop(delta_time, &game_data, &input);
        RendererDrawFrame(renderer);

        {
            // 60 fps cap
            const i64 max_frame_time = 10000000.0f / 60.f;
            i64 now = PlatformGetTicks();
            i64 frame_time_us = (now - frame_start);
            i64 sleep_time = max_frame_time - frame_time_us;

            f32 percent = (f32)frame_time_us / (f32)max_frame_time * 100.0f;

            char title[64];
            snprintf(title,
                     40,
                     "FPS : %.2f | %lldms | %2.f%% | %s",
                     10000000.0f / frame_time_us,
                     frame_time_us / 10000,
                     percent,
                     sleep_time > 0 ? "CPU" : "GPU");

            while(sleep_time > 0) {
                now = PlatformGetTicks();
                frame_time_us = (now - frame_start);
                sleep_time = max_frame_time - frame_time_us;
            }

            SetWindowText(global_window.hwnd, title);
        }
    }
    RendererDestroy(renderer);

    Win32CloseModule(&game_module);
    DBG_END();

    return 0;
}
/*
// Reload vulkan if necessary
if(Win32ShouldReloadModule(&renderer_module)) {
    pfn_DestroyRenderer(renderer);
    Win32CloseModule(&renderer_module);

    Win32LoadModule(&renderer_module, "renderer");
    RendererLoadFunctions(&renderer_module);
    renderer = pfn_CreateRenderer(window, &platform_api);
    GameLoadRendererAPI(renderer, &renderer_module, &game_data);

    sLog("Vulkan reloaded");
}
*/