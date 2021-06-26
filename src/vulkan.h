/* date = April 25th 2021 0:19 pm */

#ifndef VULKAN_H
#define VULKAN_H

struct VulkanContext;
struct Scene;

typedef VulkanContext *fn_VulkanCreateContext(SDL_Window *window);
fn_VulkanCreateContext *pfn_VulkanCreateContext;

typedef void fn_VulkanDestroyContext(VulkanContext *context);
fn_VulkanDestroyContext *pfn_VulkanDestroyContext;

typedef void fn_VulkanReloadShaders(VulkanContext *context, Scene *scene);
fn_VulkanReloadShaders *pfn_VulkanReloadShaders;

typedef void fn_VulkanDrawFrame(VulkanContext *context, Scene *scene, GameData *game_data);
fn_VulkanDrawFrame *pfn_VulkanDrawFrame;

typedef void fn_VulkanDrawRTXFrame(VulkanContext *context, Scene *scene, GameData *game_data);
fn_VulkanDrawRTXFrame *pfn_VulkanDrawRTXFrame;

typedef Scene *fn_VulkanCreateScene(VulkanContext *context);
fn_VulkanCreateScene *pfn_VulkanCreateScene;

typedef void fn_VulkanFreeScene(VulkanContext *context, Scene *scene);
fn_VulkanFreeScene *pfn_VulkanFreeScene;

#endif // VULKAN_H
