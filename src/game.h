#ifndef MAIN_H
#define MAIN_H

#include <sl3dge-utils/sl3dge.h>

#include "renderer/renderer.h"

typedef struct GameInput {
    Keyboard keyboard;
    u8 mouse;
    i32 mouse_x;
    i32 mouse_y;
} GameInput;

typedef struct GameData {
    Renderer *renderer;
    RendererGameAPI renderer_api;
    Vec3 position;
    Vec2f spherical_coordinates;
    Vec3 light_pos;
    f32 cos;
    MeshInstance moto;
} GameData;

typedef void GameStart_t(GameData *game_data);
GameStart_t *pfn_GameStart;

typedef void GameLoop_t(float delta_time, GameData *game_data, GameInput *input);
GameLoop_t *pfn_GameLoop;

#endif // MAIN_H
