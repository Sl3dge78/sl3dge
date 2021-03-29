/* date = March 23rd 2021 0:35 am */

#ifndef MAIN_H
#define MAIN_H

#include <vulkan/vulkan.h>
#include <SDL/SDL.h>

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

#define GAME_GET_SCENE(name) void name(u32 *vtx_count, vec3 *vertices, u32 *idx_count, u32 *indices)
typedef GAME_GET_SCENE(game_get_scene);
GAME_GET_SCENE(GameGetSceneStub) {}

#define GAME_LOOP(name) void name(void)
typedef GAME_LOOP(game_loop);
GAME_LOOP(GameLoopStub) { }

#endif //MAIN_H
