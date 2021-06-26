#ifndef RENDERER_H
#define RENDERER_H

#include <sl3dge/sl3dge.h>

struct Renderer;
struct Scene;
struct GameData;
struct RenderCommands;

typedef Renderer *fn_CreateRenderer(SDL_Window *window, PlatformAPI *platform_api);
fn_CreateRenderer *pfn_CreateRenderer;

typedef void fn_DestroyRenderer(Renderer *renderer);
fn_DestroyRenderer *pfn_DestroyRenderer;

typedef void fn_RendererReloadShaders(Renderer *renderer);
fn_RendererReloadShaders *pfn_ReloadShaders;

typedef void fn_DrawFrame(Renderer *renderer, GameData *game_data);
fn_DrawFrame *pfn_DrawFrame;

/*
typedef void fn_BeginFrame(Renderer *renderer);
fn_BeginFrame *pfn_BeginFrame;
typedef void fn_EndFrame(Renderer *renderer);
fn_EndFrame *pfn_EndFrame;
typedef Scene *fn_CreateScene(Renderer *renderer);
fn_CreateScene *pfn_CreateScene;
typedef void fn_DestroyScene(Renderer *renderer, Scene *scene);
fn_DestroyScene *pfn_DestroyScene;
typedef void fn_DrawRTXFrame(Renderer *renderer, Scene *scene, GameData *game_data);
fn_DrawRTXFrame *pfn_DrawRTXFrame;
*/

#endif