#ifndef RENDERER_H
#define RENDERER_H

#include <sl3dge/sl3dge.h>

struct Renderer;
struct Scene;
struct GameData;

typedef Renderer *fn_CreateRenderer(SDL_Window *window);
fn_CreateRenderer *pfn_CreateRenderer;

typedef void fn_DestroyRenderer(Renderer *renderer);
fn_DestroyRenderer *pfn_DestroyRenderer;

typedef void fn_RendererReloadShaders(Renderer *renderer, Scene *scene);
fn_RendererReloadShaders *pfn_ReloadShaders;

typedef Scene *fn_CreateScene(Renderer *renderer);
fn_CreateScene *pfn_CreateScene;

typedef void fn_DestroyScene(Renderer *renderer, Scene *scene);
fn_DestroyScene *pfn_DestroyScene;

typedef void fn_BeginFrame(Renderer *renderer);
fn_BeginFrame *pfn_BeginFrame;

typedef void fn_EndFrame(Renderer *renderer);
fn_EndFrame *pfn_EndFrame;

typedef void fn_DrawFrame(Renderer *renderer, Scene *scene, GameData *game_data);
fn_DrawFrame *pfn_DrawFrame;

/*
typedef void fn_DrawRTXFrame(Renderer *renderer, Scene *scene, GameData *game_data);
fn_DrawRTXFrame *pfn_DrawRTXFrame;
*/

void RendererLoadFunctions(Module *dll) {
    pfn_CreateRenderer = (fn_CreateRenderer *)PlatformGetProcAddress(dll, "VulkanCreateContext");
    ASSERT(pfn_CreateRenderer);
    pfn_DestroyRenderer = (fn_DestroyRenderer *)PlatformGetProcAddress(dll, "VulkanDestroyContext");
    ASSERT(pfn_DestroyRenderer);
    pfn_ReloadShaders =
        (fn_RendererReloadShaders *)PlatformGetProcAddress(dll, "VulkanReloadShaders");
    ASSERT(pfn_ReloadShaders);
    pfn_DrawFrame = (fn_DrawFrame *)PlatformGetProcAddress(dll, "VulkanDrawFrame");
    ASSERT(pfn_DrawFrame);
    pfn_CreateScene = (fn_CreateScene *)PlatformGetProcAddress(dll, "VulkanCreateScene");
    ASSERT(pfn_CreateScene);
    pfn_DestroyScene = (fn_DestroyScene *)PlatformGetProcAddress(dll, "VulkanFreeScene");
    ASSERT(pfn_DestroyScene);
}

#endif