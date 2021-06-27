// Global includes

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <sl3dge/sl3dge.h>

#include "platform.h"

#include "game.h"
#include "renderer/renderer.h"

/*
=== TODO ===

 CRITICAL

 MAJOR

 BACKLOG

 IMPROVEMENTS

 IDEAS
 - In game console
*/

typedef struct ShaderCode {
    const char *spv_path;
    FILETIME last_write_time;
} ShaderCode;

void GameLoadFunctions(Module *dll) {
    pfn_GameStart = (GameStart_t *)GetProcAddress(dll->dll, "GameStart");
    pfn_GameLoop = (GameLoop_t *)GetProcAddress(dll->dll, "GameLoop");
}

void RendererLoadFunctions(Module *dll) {
    pfn_CreateRenderer = (CreateRenderer_t *)GetProcAddress(dll->dll, "VulkanCreateRenderer");
    ASSERT(pfn_CreateRenderer);
    pfn_DestroyRenderer = (DestroyRenderer_t *)GetProcAddress(dll->dll, "VulkanDestroyRenderer");
    ASSERT(pfn_DestroyRenderer);
    pfn_ReloadShaders = (RendererReloadShaders_t *)GetProcAddress(dll->dll, "VulkanReloadShaders");
    ASSERT(pfn_ReloadShaders);
    pfn_DrawFrame = (DrawFrame_t *)GetProcAddress(dll->dll, "VulkanDrawFrame");
    ASSERT(pfn_DrawFrame);
}

internal void
LogOutput(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    FILE *std_err;
    fopen_s(&std_err, "bin/log.txt", "a");
    // fseek(std_err, 0, SEEK_END);
    fprintf(std_err, "%s\n", message);
    fclose(std_err);
}

// if result is NULL, function will query the file size for allocation in file_size.
void PlatformReadBinary(const char *path, i64 *file_size, u32 *result) {
    FILE *file;
    fopen_s(&file, path, "rb");
    if(!file) {
        SDL_LogError(0, "Unable to open file");
        SDL_LogError(0, path);
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

internal int main(int argc, char *argv[]) {
#if DEBUG
    AllocConsole();
#endif

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow(
        "Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    // Log to file
#ifndef DEBUG
    // Clear the log file
    FILE *std_err = fopen("bin/log.txt", "w+");
    fclose(std_err);
    SDL_LogSetOutputFunction(&LogOutput, NULL);
#endif

    PlatformAPI platform_api = {};
    platform_api.ReadBinary = &PlatformReadBinary;

    Module renderer_module = {};
    Win32LoadModule(&renderer_module, "renderer");
    RendererLoadFunctions(&renderer_module);

    Renderer *renderer = pfn_CreateRenderer(window, &platform_api);

    Module game_module = {};
    Win32LoadModule(&game_module, "game");
    GameLoadFunctions(&game_module);

    SDL_ShowWindow(window);

    ShaderCode shader_code = {};
    shader_code.spv_path = "resources\\shaders\\shaders.meta";
    shader_code.last_write_time = Win32GetLastWriteTime(shader_code.spv_path);

    GameData game_data = {};

    // Load Game API functions
    {
        game_data.renderer = renderer;
        game_data.renderer_api.LoadMesh =
            (LoadMesh_t *)GetProcAddress(renderer_module.dll, "RendererLoadMesh");
        ASSERT(game_data.renderer_api.LoadMesh);
        game_data.renderer_api.DestroyMesh =
            (DestroyMesh_t *)GetProcAddress(renderer_module.dll, "RendererDestroyMesh");
        ASSERT(game_data.renderer_api.DestroyMesh);
        game_data.renderer_api.InstantiateMesh =
            (InstantiateMesh_t *)GetProcAddress(renderer_module.dll, "RendererInstantiateMesh");
        ASSERT(game_data.renderer_api.InstantiateMesh);
        game_data.renderer_api.SetCamera =
            (SetCamera_t *)GetProcAddress(renderer_module.dll, "RendererSetCamera");
        ASSERT(game_data.renderer_api.SetCamera);
        game_data.renderer_api.SetSunDirection =
            (SetSunDirection_t *)GetProcAddress(renderer_module.dll, "RendererSetSunDirection");
        ASSERT(game_data.renderer_api.SetSunDirection);
    }

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

            Win32LoadModule(&renderer_module, "vulkan");
            RendererLoadFunctions(&renderer_module);
            renderer = pfn_CreateRenderer(window, &platform_api);

            SDL_Log("Vulkan reloaded");
        }

        // Reload gamecode if necessary
        if(Win32ShouldReloadModule(&game_module)) {
            Win32CloseModule(&game_module);
            Win32LoadModule(&game_module, "game");
            GameLoadFunctions(&game_module);
            SDL_Log("Game code reloaded");
        }

        // Reload shaders if necessary
        FILETIME shader_time = Win32GetLastWriteTime(shader_code.spv_path);
        if(CompareFileTime(&shader_code.last_write_time, &shader_time)) {
            shader_code.last_write_time = Win32GetLastWriteTime(shader_code.spv_path);
            pfn_ReloadShaders(renderer);
            SDL_Log("Shaders reloaded");
        }

        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                SDL_Log("Quit!");
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