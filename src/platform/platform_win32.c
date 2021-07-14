// Global includes

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <windows.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

// Surface creation
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include <sl3dge-utils/sl3dge.h>

#include "platform.h"

#include "game.h"
#include "renderer/renderer.h"

global HANDLE stderrHandle;

typedef struct ShaderCode {
    const char *spv_path;
    FILETIME last_write_time;
} ShaderCode;

typedef struct PlatformWindow {
    HWND hwnd;
    HINSTANCE hinstance;
} PlatformWindow;

void Win32GameLoadFunctions(Module *dll) {
    pfn_GameStart = (GameStart_t *)GetProcAddress(dll->dll, "GameStart");
    pfn_GameLoop = (GameLoop_t *)GetProcAddress(dll->dll, "GameLoop");
}

void Win32GameLoadRendererAPI(Renderer *renderer, Module *renderer_module, GameData *game_data) {
    game_data->renderer = renderer;
    game_data->renderer_api.LoadMesh =
        (LoadMesh_t *)GetProcAddress(renderer_module->dll, "RendererLoadMesh");
    ASSERT(game_data->renderer_api.LoadMesh);
    game_data->renderer_api.DestroyMesh =
        (DestroyMesh_t *)GetProcAddress(renderer_module->dll, "RendererDestroyMesh");
    ASSERT(game_data->renderer_api.DestroyMesh);
    game_data->renderer_api.InstantiateMesh =
        (InstantiateMesh_t *)GetProcAddress(renderer_module->dll, "RendererInstantiateMesh");
    ASSERT(game_data->renderer_api.InstantiateMesh);
    game_data->renderer_api.SetCamera =
        (SetCamera_t *)GetProcAddress(renderer_module->dll, "RendererSetCamera");
    ASSERT(game_data->renderer_api.SetCamera);
    game_data->renderer_api.SetSunDirection =
        (SetSunDirection_t *)GetProcAddress(renderer_module->dll, "RendererSetSunDirection");
    ASSERT(game_data->renderer_api.SetSunDirection);
}

void Win32RendererLoadFunctions(Module *dll) {
    pfn_CreateRenderer = (CreateRenderer_t *)GetProcAddress(dll->dll, "VulkanCreateRenderer");
    ASSERT(pfn_CreateRenderer);
    pfn_DestroyRenderer = (DestroyRenderer_t *)GetProcAddress(dll->dll, "VulkanDestroyRenderer");
    ASSERT(pfn_DestroyRenderer);
    pfn_ReloadShaders = (RendererReloadShaders_t *)GetProcAddress(dll->dll, "VulkanReloadShaders");
    ASSERT(pfn_ReloadShaders);
    pfn_DrawFrame = (DrawFrame_t *)GetProcAddress(dll->dll, "VulkanDrawFrame");
    ASSERT(pfn_DrawFrame);
}

void PlatformCreateVkSurface(VkInstance instance, PlatformWindow *window, VkSurfaceKHR *surface) {
    VkWin32SurfaceCreateInfoKHR ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    ci.pNext = NULL;
    ci.flags = 0;
    ci.hinstance = window->hinstance;
    ci.hwnd = window->hwnd;
    vkCreateWin32SurfaceKHR(instance, &ci, NULL, surface);
}

void PlatformGetInstanceExtensions(u32 *count, const char **extensions) {
    *count = 2;

    if(extensions != NULL) {
        extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
        extensions[1] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    }
}

i64 PlatformGetTicks() {
    static i64 frequency = 0;
    if(!frequency) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        frequency = freq.QuadPart;
    }

    LARGE_INTEGER ticks;
    QueryPerformanceCounter(&ticks);
    return ticks.QuadPart / frequency;
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
    case WM_DESTROY: sLog("WM_DESTROY"); return 0;
    case WM_CLOSE:
        sLog("WM_CLOSE");
        PostQuitMessage(0);
        return 0;

    default: return DefWindowProc(hwnd, msg, wparam, lparam);
    }
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

enum Win32ScanCodes {
    WIN32_Q = 0x10,
    WIN32_W = 0x11,
    WIN32_E = 0x12,
    WIN32_A = 0x1E,
    WIN32_S = 0x1F,
    WIN32_D = 0x20,
    WIN32_SPACE = 0x39
};

void Win32PrintScanCode(const char c) {
    u32 vkeycode = MapVirtualKey(LOBYTE(VkKeyScan(c)), 0);
    sLog("%c %x", c, vkeycode);
}

// There are probably better ways to handle this
// TODO input update the mouse
void Win32UpdateInput(GameInput *input) {
    BYTE win32_keyboard[256];
    GetKeyboardState(win32_keyboard);

    input->keyboard[KEY_Q] = win32_keyboard[MapVirtualKey(WIN32_Q, 1)] & 0x80;
    input->keyboard[KEY_W] = win32_keyboard[MapVirtualKey(WIN32_W, 1)] & 0x80;
    input->keyboard[KEY_E] = win32_keyboard[MapVirtualKey(WIN32_E, 1)] & 0x80;
    input->keyboard[KEY_A] = win32_keyboard[MapVirtualKey(WIN32_A, 1)] & 0x80;
    input->keyboard[KEY_S] = win32_keyboard[MapVirtualKey(WIN32_S, 1)] & 0x80;
    input->keyboard[KEY_D] = win32_keyboard[MapVirtualKey(WIN32_D, 1)] & 0x80;
}

i32 WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR cmd_line, INT cmd_show) {
    AttachConsole(ATTACH_PARENT_PROCESS);
    stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
    sLogSetCallback(&Win32Log);

    PlatformWindow window = {0};
    if(!Win32CreateWindow(instance, &window)) {
        return -1;
    }

    PlatformAPI platform_api = {0};
    platform_api.ReadBinary = &PlatformReadBinary;
    platform_api.CreateVkSurface = &PlatformCreateVkSurface;
    platform_api.GetInstanceExtensions = &PlatformGetInstanceExtensions;

    Module renderer_module = {0};
    Win32LoadModule(&renderer_module, "renderer");
    Win32RendererLoadFunctions(&renderer_module);

    Renderer *renderer = pfn_CreateRenderer(&window, &platform_api);

    Module game_module = {0};
    Win32LoadModule(&game_module, "game");
    Win32GameLoadFunctions(&game_module);

    ShaderCode shader_code = {0};
    shader_code.spv_path = "resources\\shaders\\shaders.meta";
    shader_code.last_write_time = Win32GetLastWriteTime(shader_code.spv_path);

    GameData game_data = {0};
    Win32GameLoadRendererAPI(renderer, &renderer_module, &game_data);

    ShowWindow(window.hwnd, cmd_show);
    pfn_GameStart(&game_data);

    Win32PrintScanCode('S');
    Win32PrintScanCode('Z');
    Win32PrintScanCode('D');
    Win32PrintScanCode('A');
    Win32PrintScanCode('E');
    Win32PrintScanCode(' ');

    bool running = true;
    f32 delta_time = 0;
    i64 frame_start = PlatformGetTicks();
    sLog("Running");
    while(running) {
        {
            // Sync
            i64 time = PlatformGetTicks();
            delta_time = (float)(time - frame_start) / 1000.f;
            frame_start = time;
        }

        MSG msg = {0};
        while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            BOOL value = GetMessage(&msg, NULL, 0, 0);
            if(value != 0) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else if(value == 0) {
                running = false;
                break;
            } else {
                sError("WIN32 : Message error");
            }
        }
        GameInput input = {0};
        Win32UpdateInput(&input);
        pfn_GameLoop(delta_time, &game_data, &input);
        pfn_DrawFrame(renderer);

        {
            // 60 fps cap
            i32 now = PlatformGetTicks();
            i32 frame_time_ms = (now - frame_start);
            char title[40];
            f32 idle_percent = frame_time_ms / (1000.0f / 60.0f);
            idle_percent = 1 - idle_percent;
            idle_percent *= 100.0f;
            snprintf(title, 40, "Frametime : %dms Idle: %2.f%%", frame_time_ms, idle_percent);
            SetWindowText(window.hwnd, title);
            if(frame_time_ms < 1.0f / 0.06) {
                Sleep(16.6 - frame_time_ms);
            }
        }
    }
    pfn_DestroyRenderer(renderer);

    Win32CloseModule(&game_module);
    Win32CloseModule(&renderer_module);
    DBG_END();

    return 0;
}

/*

internal int main(int argc, char *argv[]) {
#if DEBUG
    //AllocConsole(); // This might be needed
    AttachConsole(ATTACH_PARENT_PROCESS);
    stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
#endif

    sLogSetCallback(&Win32Log);
    sLogLevel(LOG_LEVEL_LOG);

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow(
        "Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    PlatformAPI platform_api = {0};
    platform_api.ReadBinary = &PlatformReadBinary;

    Module renderer_module = {0};
    Win32LoadModule(&renderer_module, "renderer");
    RendererLoadFunctions(&renderer_module);

    Renderer *renderer = pfn_CreateRenderer(window, &platform_api);

    Module game_module = {0};
    Win32LoadModule(&game_module, "game");
    GameLoadFunctions(&game_module);

    SDL_ShowWindow(window);

    ShaderCode shader_code = {0};
    shader_code.spv_path = "resources\\shaders\\shaders.meta";
    shader_code.last_write_time = Win32GetLastWriteTime(shader_code.spv_path);

    GameData game_data = {0};

    // Load Game API functions
    GameLoadRendererAPI(renderer, &renderer_module, &game_data);
    pfn_GameStart(&game_data);

    bool running = true;
    float delta_time = 0;
    int frame_start = SDL_GetTicks();

    SDL_Event event;
    while(running) {
        {
            // Sync
            int time = SDL_GetTicks();
            delta_time = (float)(time - frame_start) / 1000.f;
            frame_start = time;
        }

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


        // Reload gamecode if necessary
        if(Win32ShouldReloadModule(&game_module)) {
            Win32CloseModule(&game_module);
            Win32LoadModule(&game_module, "game");
            GameLoadFunctions(&game_module);
            sTrace("Game code reloaded");
        }

        // Reload shaders if necessary
        FILETIME shader_time = Win32GetLastWriteTime(shader_code.spv_path);
        if(CompareFileTime(&shader_code.last_write_time, &shader_time)) {
            shader_code.last_write_time = Win32GetLastWriteTime(shader_code.spv_path);
            pfn_ReloadShaders(renderer);
            sTrace("Shaders reloaded");
        }

        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                sLog("Quit!");
                SDL_HideWindow(window);
                running = false;
            }
        }

        pfn_GameLoop(delta_time, &game_data);
        pfn_DrawFrame(renderer);

        {
            // 60 fps cap

            i32 now = SDL_GetTicks();
            i32 frame_time_ms = now - frame_start;
            char title[40];
            f32 idle_percent = frame_time_ms / (1000.0f / 60.0f);
            idle_percent = 1 - idle_percent;
            idle_percent *= 100.0f;
            snprintf(title, 40, "Frametime : %dms Idle: %2.f%%", frame_time_ms, idle_percent);
            SDL_SetWindowTitle(window, title);
            if(frame_time_ms < 1.0f / 0.06) {
                Sleep(16.6 - frame_time_ms);
            }
        }
    }

    pfn_DestroyRenderer(renderer);

    Win32CloseModule(&game_module);

    Win32CloseModule(&renderer_module);

    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    DBG_END();

    return (0);
}
*/