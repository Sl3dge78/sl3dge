// Global includes
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <vulkan/vulkan.h>

#include "game.h"

#include <windows.h>

/* 
=== TODO ===

 CRITICAL

 MAJOR

 BACKLOG

 IMPROVEMENTS

 IDEAS
 - In game console

*/

u32* Win32AllocateAndLoadBinary(const char* path, i64 *file_size);

typedef struct GameCode {
    char *dll_path;
    FILETIME last_write_time;
    HMODULE game_dll;
    
    game_get_scene* GetScene;
    game_loop* GameLoop;
    game_start* GameStart;
} GameCode;

#include "gltf.cpp"
#include "vulkan.cpp"
//#include "vulkan_rtx.cpp"

typedef struct ShaderCode {
    char *spv_path;
    FILETIME last_write_time;
} ShaderCode;

internal inline FILETIME Win32GetLastWriteTime(char *file_name) {
    
    FILETIME last_write_time = {};
    
    WIN32_FIND_DATA data;
    HANDLE handle = FindFirstFile(file_name, &data);
    if(handle != INVALID_HANDLE_VALUE) {
        last_write_time = data.ftLastWriteTime;
        FindClose(handle);
    }
    
    return last_write_time;
}

// Returns true if it loaded new gamecode
internal bool Win32LoadGameCode(GameCode* game_code) {
    
    bool result = false;
    
    game_code->dll_path = "bin\\game.dll";
    
    game_code->GetScene = GameGetSceneStub;
    game_code->GameLoop = GameLoopStub;
    game_code->GameStart = GameStartStub;
    
    // Compiler can still be writing to the file
    // Try to copy, if we can do it, compiler is done so we can load it
    // If not, compiler is not done, so copy fails and we're loading the previous one
    if(CopyFile(game_code->dll_path, "bin\\game_temp.dll", FALSE)) {
        // We were able to copy
        game_code->last_write_time = Win32GetLastWriteTime(game_code->dll_path); 
        result = true;
    } else {
        result = false;
    }
    // If we were able to copy, this'll be the new one, if we weren't able to it'll be the old one
    game_code->game_dll = LoadLibrary("bin\\game_temp.dll");
    
    if(game_code->game_dll) {
        // Load function pointers
        game_code->GetScene = (game_get_scene*)GetProcAddress(game_code->game_dll, "GameGetScene");
        game_code->GameLoop = (game_loop *) GetProcAddress(game_code->game_dll, "GameLoop");
        game_code->GameStart = (game_start *) GetProcAddress(game_code->game_dll, "GameStart");
    }
    
    return result;
}

internal void Win32UnloadGameCode(GameCode* game_code) {
    if(game_code->game_dll) {
        FreeLibrary(game_code->game_dll);
    }
    game_code->game_dll = NULL;
    game_code->GetScene = GameGetSceneStub;
    game_code->GameLoop = GameLoopStub;
    game_code->GameStart = GameStartStub;
}

// This allocates memory, free it!
u32* Win32AllocateAndLoadBinary(const char* path, i64 *file_size) {
    
    FILE *file = fopen(path,"rb");
    if(!file) {
        SDL_LogError(0, "Unable to open file");
        SDL_LogError(0, path);
    }
    // Get the size
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    rewind(file);
    
    // Copy into result
    u32 *result = (u32 *)malloc(*file_size);
    fread(result, 1, *file_size, file);
    
    fclose(file);
    return result;
}

internal int main(int argc, char *argv[]) {
    
#if DEBUG
    AllocConsole();
#endif
    
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window* window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN);
    
    VulkanContext *context = VulkanCreateContext(window);
    
    GameCode game_code = {};
    Win32LoadGameCode(&game_code);
    
    VulkanRenderer *renderer = VulkanCreateRenderer(context);
    
    SDL_Log("Vulkan Succesfully Loaded!");
    SDL_ShowWindow(window);
    
    ShaderCode shader_code = {};
    shader_code.spv_path = "resources\\shaders\\shaders.meta";
    shader_code.last_write_time = Win32GetLastWriteTime(shader_code.spv_path);
    
    GameData game_data = {};
    
    VulkanLoadGLTF("resources/models/gltf_samples/OrientationTest/glTF/OrientationTest.gltf", context, &game_data.transforms);
    //VulkanLoadGLTF("resources/models/cube_plane.gltf", context, &game_data.transforms);
    
    game_code.GameStart(&game_data);
    
    bool running = true;
    float delta_time = 0;
    int last_time = SDL_GetTicks();
    
    SDL_Event event;
    while (running) {
        
        // Synch
        int time = SDL_GetTicks();
        delta_time = (float)(time - last_time) / 1000.f;
        last_time = time;
        
        // Reload shaders if necessary
        FILETIME shader_time = Win32GetLastWriteTime(shader_code.spv_path);
        if(CompareFileTime(& shader_code.last_write_time, &shader_time)){
            shader_code.last_write_time = Win32GetLastWriteTime(shader_code.spv_path);
            VulkanReloadShaders(context, renderer);
            SDL_Log("Shaders reloaded");
        }
        
        // Reload gamecode if necessary
        FILETIME game_code_time = Win32GetLastWriteTime(game_code.dll_path);
        if(CompareFileTime(&game_code.last_write_time, &game_code_time)){
            Win32UnloadGameCode(&game_code);
            if(Win32LoadGameCode(&game_code)) {
                SDL_Log("Game code successfully reloaded");
            }
        }
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                SDL_Log("Quit!");
                SDL_HideWindow(window);
                running = false;
            }
        }
        game_code.GameLoop(delta_time, &game_data);
        VulkanUpdateDescriptors(context, &game_data);
        VulkanDrawFrame(context, renderer, &game_data);
    }
    
    VulkanFreeGLTF(context, &game_data.transforms);
    
    Win32UnloadGameCode(&game_code);
    
    VulkanDestroyRenderer(context, renderer);
    VulkanDestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    DBG_END();
    return (0);
}