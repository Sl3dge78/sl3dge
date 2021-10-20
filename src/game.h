#ifndef MAIN_H
#define MAIN_H

#include "utils/sl3dge.h"

#include "game_api.h"
#include "renderer/renderer.h"
#include "console.h"
#include "event.h"

typedef struct Camera {
    Vec3 position;
    Vec2f spherical_coordinates;
    Vec3 forward;
} Camera;

typedef struct Animation {
    u32 key_count;
    f32 length;
    f32 *key_times;
    Quat *quat_keys;
} Animation;

typedef struct GameData {
    // System stuff
    Console console;
    EventQueue event_queue;
    
    bool is_free_cam;
    Camera camera;
    
    Vec3 light_dir;
    f32 cos;
    
    MeshHandle floor;
    MeshHandle box;
    MeshHandle character;
    
    Transform *floor_xform;
    Transform *npc_xform;
    
    Vec3 interact_sphere_pos;
    f32 interact_sphere_diameter;
    
    MeshHandle simple_skinning;
    Transform *simple_skinning_root;
    SkinnedMeshHandle skinned_mesh;
    
    f32 anim_time;
    Animation anim;
    
} GameData;

DLL_EXPORT GameGetSize_t GameGetSize;
DLL_EXPORT GameStart_t GameStart;
DLL_EXPORT GameLoop_t GameLoop;
DLL_EXPORT GameLoad_t GameLoad;
DLL_EXPORT GameEnd_t GameEnd;

#endif // MAIN_H
