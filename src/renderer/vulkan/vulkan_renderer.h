#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vulkan/vulkan.h>
#include <sl3dge-utils/sl3dge.h>

#define VK_DECL_FUNC(name) static PFN_##name pfn_##name
#define VK_LOAD_INSTANCE_FUNC(instance, name)                                                      \
    pfn_##name = (PFN_##name)vkGetInstanceProcAddr(instance, #name);                               \
    ASSERT(pfn_##name);
#define VK_LOAD_DEVICE_FUNC(name)                                                                  \
    pfn_##name = (PFN_##name)vkGetDeviceProcAddr(device, #name);                                   \
    ASSERT(pfn_##name);

VK_DECL_FUNC(vkCreateDebugUtilsMessengerEXT);
VK_DECL_FUNC(vkDestroyDebugUtilsMessengerEXT);

VK_DECL_FUNC(vkCreateRayTracingPipelinesKHR);
VK_DECL_FUNC(vkCmdTraceRaysKHR);
VK_DECL_FUNC(vkGetRayTracingShaderGroupHandlesKHR);
VK_DECL_FUNC(vkCreateAccelerationStructureKHR);
VK_DECL_FUNC(vkGetAccelerationStructureBuildSizesKHR);
VK_DECL_FUNC(vkCmdBuildAccelerationStructuresKHR);
VK_DECL_FUNC(vkDestroyAccelerationStructureKHR);
VK_DECL_FUNC(vkGetAccelerationStructureDeviceAddressKHR);

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

typedef struct RenderGroup {
    VkRenderPass render_pass;
    VkPipelineLayout layout;
    u32 descriptor_set_count;
    VkDescriptorSetLayout *set_layouts;
    VkDescriptorSet *descriptor_sets;
    VkPipeline pipeline;

    u32 clear_values_count;
    VkClearValue *clear_values;
} RenderGroup;

typedef struct Mesh {
    Buffer *buffer;
    u32 all_index_offset; // Indices start at this offset in the buffer

    u32 total_vertex_count;
    u32 total_index_count;
    u32 total_primitives_count;
    Primitive *primitives;

    u32 primitive_nodes_count;
    Mat4 *primitive_transforms;

    u32 instance_count;
    u32 instance_capacity;
    Mat4 *instance_transforms;
} Mesh;

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
    VkFormat depth_format;

    VkSampler texture_sampler;

    Image depth_image;

    VkSampler depth_sampler;
    Image resolved_depth_image;

    Image msaa_image;

    Buffer camera_info_buffer;
    CameraMatrices camera_info;

    Image shadowmap;
    VkExtent2D shadowmap_extent;
    VkSampler shadowmap_sampler;
    VkFramebuffer shadowmap_framebuffer;
    RenderGroup shadowmap_render_group;

    RenderGroup main_render_group;
    // Do we need three of these ?
    VkFramebuffer color_pass_framebuffer;
    Image color_pass_image;

    RenderGroup volumetric_render_group;
    VkFramebuffer *framebuffers;

    u32 materials_count;
    Buffer mat_buffer;

    u32 textures_capacity;
    u32 textures_count;
    Image *textures;

    u32 mesh_capacity;
    u32 mesh_count;
    Buffer **meshes_buffer; // idx & vtx buffer
    Mesh **meshes;

} Renderer;

struct Frame {
    VkCommandBuffer cmd;
    VkPipelineLayout layout;
};

void DestroyRenderGroup(Renderer *context, RenderGroup *render_group);
void CreateMainRenderGroup(Renderer *renderer, RenderGroup *render_group);
void VulkanUpdateTextureDescriptorSet(VkDevice device,
                                      VkDescriptorSet set,
                                      VkSampler sampler,
                                      const u32 texture_count,
                                      Image *textures);

#endif