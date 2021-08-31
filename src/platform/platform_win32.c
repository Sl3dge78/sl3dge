// Global includes

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define SL3DGE_IMPLEMENTATION
#define LEAK_TEST
#include <sl3dge-utils/sl3dge.h>

#include "platform.h"
#include "platform_win32.h"

#include "game_api.h"
#include "renderer/renderer_api.h"

typedef struct ShaderCode {
    const char *spv_path;
    FILETIME last_write_time;
} ShaderCode;

/* GLOBALS */

global HANDLE stderrHandle;
global bool mouse_captured; // @TODO this probably shouldn't be a global
global Renderer *renderer;  //Global to have it in the window proc
global PlatformWindow global_window;
global PlatformAPI platform_api;
global bool running;

/* FUNCTIONS */
void Win32GameLoadFunctions(Module *dll) {
    pfn_GameGetSize = (GameGetSize_t *)GetProcAddress(dll->dll, "GameGetSize");
    pfn_GameLoad = (GameLoad_t *)GetProcAddress(dll->dll, "GameLoad");
    pfn_GameStart = (GameStart_t *)GetProcAddress(dll->dll, "GameStart");
    pfn_GameEnd = (GameEnd_t *)GetProcAddress(dll->dll, "GameEnd");
    pfn_GameLoop = (GameLoop_t *)GetProcAddress(dll->dll, "GameLoop");
    sLogSetCallback((sLogCallback_t *)GetProcAddress(dll->dll, "ConsoleLogMessage"));
}

void Win32RendererLoadFunctions(Module *dll) {
    pfn_GetRendererSize = (GetRendererSize_t *)GetProcAddress(dll->dll, "GetRendererSize");
    pfn_RendererInit = (RendererInit_t *)GetProcAddress(dll->dll, "RendererInit");
    pfn_RendererDrawFrame = (RendererDrawFrame_t *)GetProcAddress(dll->dll, "RendererDrawFrame");
    pfn_RendererDestroy = (RendererDestroy_t *)GetProcAddress(dll->dll, "RendererDestroy");
    pfn_RendererUpdateWindow = (RendererUpdateWindow_t *)GetProcAddress(dll->dll, "RendererUpdateWindow");
}

void Win32LoadFunctions(Module *dll) {
    Win32GameLoadFunctions(dll);
    Win32RendererLoadFunctions(dll);
}

void PlatformRequestExit() {
    running = false;
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

void PlatformReadWholeFile(const char *path, i32 *file_size, char *dest) {
    FILE *file;
    fopen_s(&file, path, "r");
    if(!file) {
        sError(0, "Unable to open file");
        sError(0, path);
        return;
    }
    if(dest == NULL) {
        // Get the size
        fseek(file, 0, SEEK_END);
        *file_size = ftell(file);
        fclose(file);
        return;
    } else {
        // Copy into result
        fread(dest, 1, *file_size, file);
        fclose(file);
        return;
    }
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

// @TODO : Handle UTF8
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
    case WM_CLOSE: PostQuitMessage(0); return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        sError("Keydown intercepted in window proc, this shouldn't happen");
        return 0;
    case WM_SIZE:
    case WM_SIZING: {
        RECT rect;
        GetClientRect(global_window.hwnd, &rect);
        pfn_RendererUpdateWindow(renderer, &platform_api, rect.right, rect.bottom);
        return 0;
    }
    default: return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

void Win32HandleKeyboardMessages(WPARAM wParam, LPARAM lParam, Input *input) {
    // 2149 5809
    u32 scancode = (lParam >> 16) & 0x7F;
    bool is_down = !(lParam & (1 << 31));

    //sLog("%x", scancode);

    input->keyboard[scancode] = is_down;

    if(scancode == SCANCODE_ARRET_DEFIL) {
        DBG_keep_console_open = true;
        sLog("Console will stay open");
    }

    if(is_down && input->read_text_input) {
        input->text_input = wParam;
    }
}

bool Win32CreateWindow(HINSTANCE instance, PlatformWindow *window) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = Win32WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = "Vulkan";
    RegisterClass(&wc);

    window->w = 1280;
    window->h = 720;
    HWND hwnd = CreateWindow("Vulkan",
                             "Vulkan",
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             window->w,
                             window->h,
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
    window->dc = GetDC(hwnd);

    return true;
}

i32 WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR cmd_line, INT cmd_show) {
    Leak_Begin();

    AllocConsole();

    stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
    sLogSetCallback(&Win32Log);

    if(!Win32CreateWindow(instance, &global_window)) {
        return -1;
    }

    platform_api.ReadBinary = &PlatformReadBinary;
    platform_api.SetCaptureMouse = &PlatformSetCaptureMouse;
    platform_api.ReadWholeFile = &PlatformReadWholeFile;
    platform_api.SetCaptureMouse = &PlatformSetCaptureMouse;
    platform_api.RequestExit = &PlatformRequestExit;
    platform_api.DebugInfo = Leak_GetList();

    Module game_module = {0};
    Win32LoadModule(&game_module, "game");
    Win32LoadFunctions(&game_module);

    GameData *game_data = sCalloc(1, pfn_GameGetSize());
    renderer = sCalloc(1, pfn_GetRendererSize());
    Input *input = sCalloc(1, sizeof(Input));

    pfn_GameLoad(game_data, renderer, &platform_api);
    pfn_RendererInit(renderer, &platform_api, &global_window);

    ShowWindow(global_window.hwnd, cmd_show);
    SetForegroundWindow(global_window.hwnd);
    pfn_GameStart(game_data);

    running = true;
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

        { // Hot Reloading
            if(Win32ShouldReloadModule(&game_module)) {
                Win32CloseModule(&game_module);
                Win32LoadModule(&game_module, "game");
                Win32LoadFunctions(&game_module);
                pfn_GameLoad(game_data, renderer, &platform_api);
                sLog("Game code reloaded");
            }
        }

        RECT window_size;
        GetClientRect(global_window.hwnd, &window_size);

        memcpy(input->old_keyboard, input->keyboard, sizeof(Keyboard));

        { // Mouse Position
            POINT pos;
            GetCursorPos(&pos);
            ScreenToClient(global_window.hwnd, &pos);
            input->mouse_delta_x = pos.x - input->mouse_x;
            input->mouse_delta_y = pos.y - input->mouse_y;

            if(mouse_captured) {
                u32 center_x = window_size.right / 2;
                u32 center_y = window_size.bottom / 2;
                input->mouse_x = center_x;
                input->mouse_y = center_y;
                POINT screen_pos = {center_x, center_y};
                ClientToScreen(global_window.hwnd, &screen_pos);
                SetCursorPos(screen_pos.x, screen_pos.y);
            } else {
                input->mouse_x = pos.x;
                input->mouse_y = pos.y;
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
                case WM_SYSKEYUP:
                case WM_CHAR:
                    Win32HandleKeyboardMessages(msg.wParam, msg.lParam, input);
                    break;

                case WM_LBUTTONDOWN: input->mouse |= MOUSE_LEFT; break;
                case WM_LBUTTONUP: input->mouse &= ~MOUSE_LEFT; break;
                case WM_MBUTTONDOWN: input->mouse |= MOUSE_MIDDLE; break;
                case WM_MBUTTONUP: input->mouse &= ~MOUSE_MIDDLE; break;
                case WM_RBUTTONDOWN: input->mouse |= MOUSE_RIGHT; break;
                case WM_RBUTTONUP: input->mouse &= ~MOUSE_RIGHT; break;
                case WM_EXITSIZEMOVE: sLog("exit!"); break;
                default:
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

        pfn_GameLoop(delta_time, game_data, input);
        pfn_RendererDrawFrame(renderer);

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

    sLogSetCallback(&Win32Log);
    pfn_RendererDestroy(renderer);
    pfn_GameEnd(game_data);

    sFree(renderer);
    sFree(game_data);
    sFree(input);

    DestroyWindow(global_window.hwnd);
    ReleaseDC(global_window.hwnd, global_window.dc);

    Leak_End();

    Win32CloseModule(&game_module);

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
