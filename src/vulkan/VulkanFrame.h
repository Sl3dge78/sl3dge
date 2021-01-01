#pragma once

#include <vulkan/vulkan.hpp>

struct VulkanFrame {
	VkDevice device;
	VkImage image;
	VkImageView image_view;

	VkFramebuffer framebuffer;

	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;

	VkFence fence;

	VkDescriptorSet descriptor_set;
	VkBuffer uniform_buffer;
	VkDeviceMemory uniform_buffer_memory;

	void init_frame(VkDevice device);
	void create_framebuffer(VkExtent2D swapchain_extent, vk::RenderPass &render_pass, VkImageView &depth_image_view);
	void create_command_buffers(VkCommandPool command_pool);
	void create_sync_objects();
	void create_descriptor_set(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, VkSampler texture_sampler, VkImageView texture_image_view);
	void create_uniform_buffer(VkPhysicalDevice physical_device);
	void delete_frame();
};
