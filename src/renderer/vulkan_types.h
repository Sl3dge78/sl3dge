#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include <vulkan/vulkan.h>
#include <sl3dge/sl3dge.h>

#include "platform/platform.h"

typedef struct Buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceAddress address;
    VkDeviceSize size;
} Buffer;

typedef struct Image {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView image_view;
} Image;

typedef struct VulkanLayout {
    VkPipelineLayout layout;

    VkDescriptorSetLayout *set_layout;

    u32 descriptor_set_count;
    VkDescriptorSet *descriptor_set;

    u32 push_constant_size;

} VulkanLayout;

typedef struct Swapchain {
    VkSwapchainKHR swapchain;
    u32 image_count;
    VkFormat format;
    VkExtent2D extent;

    VkImage *images;
    VkImageView *image_views;

    VkCommandBuffer *command_buffers;
    VkFence *fences;

    VkSemaphore *image_acquired_semaphore;
    VkSemaphore *render_complete_semaphore;
    u32 semaphore_id;

} Swapchain;

struct RenderGroup {
    VkRenderPass render_pass;
    VkPipelineLayout layout;
    u32 descriptor_set_count;
    VkDescriptorSetLayout *set_layouts;
    VkDescriptorSet *descriptor_sets;
    VkPipeline pipeline;

    u32 clear_values_count;
    VkClearValue *clear_values;
};

typedef struct Renderer {
    PlatformAPI *platform;

    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkSurfaceKHR surface;

    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceProperties physical_device_properties;

    VkSampleCountFlagBits msaa_level;

    VkDevice device;
    u32 graphics_queue_id;
    VkQueue graphics_queue;
    u32 transfer_queue_id;
    VkQueue transfer_queue;
    u32 present_queue_id;
    VkQueue present_queue;

    VkCommandPool graphics_command_pool;
    VkDescriptorPool descriptor_pool;

    Swapchain swapchain;

    VkSampler texture_sampler;
    VkFramebuffer *framebuffers;

    Image depth_image;
    Image msaa_image;

    Buffer cam_buffer;

    Image shadowmap;
    VkExtent2D shadowmap_extent;
    VkSampler shadowmap_sampler;
    VkFramebuffer shadowmap_framebuffer;
    RenderGroup shadowmap_render_group;

    // TODO: move that in to a rendergroup
    RenderGroup main_render_group;

    u32 materials_count;
    Buffer mat_buffer;

    u32 textures_count;
    Image *textures;

    u32 mesh_count;
    Mesh **meshes;

} Renderer;

typedef struct Primitive {
    u32 material_id;
    u32 node_id;
    u32 index_count;
    u32 index_offset;
    u32 vertex_count;
    u32 vertex_offset;
} Primitive;

typedef struct PushConstant {
    alignas(16) Mat4 transform;
    alignas(4) u32 material;
} PushConstant;

typedef struct Vertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
} Vertex;

typedef struct Material {
    alignas(16) Vec3 base_color;
    alignas(4) u32 base_color_texture;
    alignas(4) u32 metallic_roughness_texture;
    alignas(4) float metallic_factor;
    alignas(4) float roughness_factor;
    alignas(4) u32 normal_texture;
    alignas(4) u32 ao_texture;
    alignas(4) u32 emissive_texture;
} Material;

struct Frame {
    VkCommandBuffer cmd;
    VkPipelineLayout layout;
};

#endif