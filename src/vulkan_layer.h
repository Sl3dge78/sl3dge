/* date = April 24th 2021 6:29 pm */

#ifndef VULKAN_LAYER_H
#define VULKAN_LAYER_H

#include <sl_math.h>

#define DECL_FUNC(name) global PFN_##name pfn_##name
#define LOAD_INSTANCE_FUNC(instance, name)                           \
	pfn_##name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
	ASSERT(pfn_##name)
#define LOAD_DEVICE_FUNC(name)                                   \
	pfn_##name = (PFN_##name)vkGetDeviceProcAddr(device, #name); \
	ASSERT(pfn_##name)

DECL_FUNC(vkCreateDebugUtilsMessengerEXT);
DECL_FUNC(vkDestroyDebugUtilsMessengerEXT);
DECL_FUNC(vkSetDebugUtilsObjectNameEXT);

DECL_FUNC(vkGetBufferDeviceAddressKHR);
DECL_FUNC(vkCreateRayTracingPipelinesKHR);
DECL_FUNC(vkCmdTraceRaysKHR);
DECL_FUNC(vkGetRayTracingShaderGroupHandlesKHR);
DECL_FUNC(vkCreateAccelerationStructureKHR);
DECL_FUNC(vkGetAccelerationStructureBuildSizesKHR);
DECL_FUNC(vkCmdBuildAccelerationStructuresKHR);
DECL_FUNC(vkDestroyAccelerationStructureKHR);
DECL_FUNC(vkGetAccelerationStructureDeviceAddressKHR);

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

typedef struct PushConstant {
	alignas(16) Mat4 transform;
	alignas(4) u32 material;
	alignas(4) u32 color_texture;
} PushConstant;

internal void AssertVkResult(VkResult result);
internal void CreateVkShaderModule(const char *path, VkDevice device, VkShaderModule *shader_module);
internal void DestroyLayout(VkDevice device, VkDescriptorPool pool, VulkanLayout *layout);

#endif //VULKAN_LAYER_H
