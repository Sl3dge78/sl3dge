#ifndef MAIN_H
#define MAIN_H

#include <sl3dge-utils/sl3dge.h>

#include "game_api.h"
#include "renderer/renderer.h"
#include "console.h"
#include "event.h"

typedef struct Camera {
    Vec3 position;
    Vec2f spherical_coordinates;
} Camera;

typedef struct GameData {
    // System stuff
    Console console;
    EventQueue event_queue;

    bool is_free_cam;
    Camera camera;

    Vec3 light_pos;
    f32 cos;

    MeshHandle bike;
    Mat4 moto_xform;
    Vec3 bike_forward;
    f32 bike_x;
    f32 bike_dir;
    f32 bank_angle;

    MeshHandle cube;
    Mat4 cube_xforms[64];
} GameData;

DLL_EXPORT GameGetSize_t GameGetSize;
DLL_EXPORT GameStart_t GameStart;
DLL_EXPORT GameLoop_t GameLoop;
DLL_EXPORT GameLoad_t GameLoad;

#endif // MAIN_H
