/* date = March 23rd 2021 0:35 am */

#ifndef MAIN_H
#define MAIN_H

typedef struct CameraMatrices {
    alignas(16) mat4 proj;
    alignas(16) mat4 view;
    alignas(16) mat4 shadow_mvp;
    alignas(16) vec3 pos;
    alignas(16) vec3 light_dir;
} CameraMatrices;

typedef struct GameData {
    CameraMatrices matrices;
    vec3 position;
    vec2 spherical_coordinates;
    vec3 light_pos;
    float cos;
} GameData;

typedef void fn_GameStart(GameData *game_data);
fn_GameStart *pfn_GameStart;

typedef void fn_GameLoop(float delta_time, GameData *game_data);
fn_GameLoop *pfn_GameLoop;

#endif //MAIN_H
