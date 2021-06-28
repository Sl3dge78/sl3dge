#ifndef MAIN_H
#define MAIN_H

#include <sl3dge/sl3dge.h>

#include "renderer/renderer.h"

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

typedef void GameLoop_t(float delta_time, GameData *game_data);
GameLoop_t *pfn_GameLoop;

#endif // MAIN_H
