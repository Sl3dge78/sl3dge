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

#define GAME_LOOP(name) void name(void)
typedef GAME_LOOP(game_loop);
GAME_LOOP(GameLoopStub) { }

#define GAME_GET_DESCRIPTORS_INFO(name) void name(u32 *count, VkDescriptorSetLayoutBinding *bindings)
typedef GAME_GET_DESCRIPTORS_INFO(game_get_descriptors_info);
GAME_GET_DESCRIPTORS_INFO(GameGetDescriptorsInfoStub) {}

#define GAME_GET_DESCRIPTORS_WRITES(name) void name(VkDescriptorSet descriptor_set, u32 *count, VkWriteDescriptorSet *writes)
typedef GAME_GET_DESCRIPTORS_WRITES(game_get_descriptor_writes);
GAME_GET_DESCRIPTORS_WRITES(GameGetDescriptorWritesStub) {}

#define GAME_GET_POOL_SIZES(name) void name(u32 *count, VkDescriptorPoolSize *pool_sizes)
typedef GAME_GET_POOL_SIZES(game_get_pool_sizes);
GAME_GET_POOL_SIZES(GameGetPoolSizesStub) {}

#define GAME_GET_PUSH_CONSTANTS(name) void name(VkPushConstantRange *push_constants)
typedef GAME_GET_PUSH_CONSTANTS(game_get_push_constants);
GAME_GET_PUSH_CONSTANTS(GameGetPushConstantsStub) {}

#endif //MAIN_H
