#ifndef MAIN_H
#define MAIN_H

#include <sl3dge-utils/sl3dge.h>

#include "renderer/renderer.h"
#include "console.h"

typedef struct GameData {
    Renderer *renderer;
    RendererGameAPI renderer_api;
    PlatformAPI platform_api;
    PFN_LogCallback logger;
    u32 window_width;
    u32 window_height;
    Vec3 position;
    Vec2f spherical_coordinates;
    Vec3 light_pos;
    f32 cos;
    MeshInstance moto;
    PushBuffer *ui_push_buffer;
    Console console;
} GameData;

typedef void GameStart_t(GameData *game_data);
GameStart_t *pfn_GameStart;

typedef void GameLoop_t(float delta_time, GameData *game_data, GameInput *input);
GameLoop_t *pfn_GameLoop;

internal void UIPushQuad(PushBuffer *push_buffer,
                         const u32 x,
                         const u32 y,
                         const u32 w,
                         const u32 h,
                         const Vec4 color);
internal void UIPushText(PushBuffer *push_buffer, const char *text, const u32 x, const u32 y, const Vec4 color); 
#endif // MAIN_H
