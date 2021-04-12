/* date = March 23rd 2021 0:35 am */

#ifndef MAIN_H
#define MAIN_H

#include <vulkan/vulkan.h>
#include <SDL/SDL.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#define internal static;
#define global static;
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t i64;
typedef int32_t i32;

#include "debug.cpp"
#include "math.cpp"

typedef struct PushConstant {
    alignas(16) vec4 clear_color;
    alignas(16) vec3 light_dir;
    alignas(4) float light_instensity;
    alignas(16) vec3 light_color;
} PushConstant;

typedef struct CameraMatrices {
    alignas(16) mat4 proj;
    alignas(16) mat4 view;
    alignas(16) mat4 mesh;
} CameraMatrices;

typedef struct GameData {
    CameraMatrices matrices;
    vec3 position;
    vec2 spherical_coordinates;
} GameData ;

#define GAME_GET_SCENE(name) void name(u32 *vtx_count, vec3 *vertices, u32 *idx_count, u32 *indices)
typedef GAME_GET_SCENE(game_get_scene);
GAME_GET_SCENE(GameGetSceneStub) {}

#define GAME_LOOP(name) void name(float delta_time, GameData *game_data)
typedef GAME_LOOP(game_loop);
GAME_LOOP(GameLoopStub) { }

#define GAME_START(name) void name(GameData *game_data)
typedef GAME_START(game_start);
GAME_START(GameStartStub) { }

#endif //MAIN_H
