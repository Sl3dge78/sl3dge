#ifndef VULKANFRAME_H
#define VULKANFRAME_H

#include <array>

#include <vulkan/vulkan.hpp>

#include "scene/Mesh.h"
#include "vulkan/VulkanHelper.h"

struct VulkanFrame {
	vk::Device device;
	vk::RenderPass render_pass;
	vk::Extent2D swapchain_extent;

	vk::UniqueImageView image_view;
	vk::UniqueFramebuffer framebuffer;
	vk::UniqueCommandBuffer command_buffer;
	vk::UniqueFence fence;

	void init_frame(vk::Device device);
	void create_framebuffer(vk::Extent2D extent, vk::RenderPass &render_pass);
	void create_command_buffers(vk::CommandPool command_pool);
	void create_sync_objects();

	void begin_render_pass();
	void end_render_pass();
};

#endif
