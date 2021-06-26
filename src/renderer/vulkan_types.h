#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include <vulkan/vulkan.h>
#include <sl3dge/sl3dge.h>

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

    VkDescriptorSetLayout set_layout;

    u32 descriptor_set_count;
    VkDescriptorSet descriptor_set;

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

typedef struct VulkanContext {
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

    VkRenderPass render_pass;
    VkSampler texture_sampler;
    VkFramebuffer *framebuffers;

    Image depth_image;
    Image msaa_image;

    Buffer cam_buffer;

    Image shadowmap;
    VkExtent2D shadowmap_extent;
    VkSampler shadowmap_sampler;
    VkRenderPass shadowmap_render_pass;
    VulkanLayout shadowmap_layout;
    VkPipeline shadowmap_pipeline;
    VkFramebuffer shadowmap_framebuffer;

} VulkanContext;

typedef struct Primitive {
    u32 material_id;
    u32 node_id;
    u32 index_count;
    u32 index_offset;
    u32 vertex_count;
    u32 vertex_offset;
} Primitive;

struct Mesh {
    Buffer buffer;
    u32 all_index_offset; // Indices start at this offset in the buffer

    u32 total_vertex_count;
    u32 total_index_count;
    u32 total_primitives_count;
    Primitive *primitives;

    u32 primitive_nodes_count;
    Mat4 *primitive_transforms;

    u32 instance_count;
    Mat4 *instance_transforms;
};

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

typedef struct Scene {
    VkPipeline pipeline;

    VkPipelineLayout layout;
    u32 descriptor_set_count;
    VkDescriptorSetLayout *set_layouts;
    VkDescriptorSet *descriptor_sets;

    u32 materials_count;
    Buffer mat_buffer;

    u32 textures_count;
    Image *textures;

    u32 mesh_count;
    Mesh **meshes;

} Scene;

#endif