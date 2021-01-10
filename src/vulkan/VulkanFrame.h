#ifndef VULKANFRAME_H
#define VULKANFRAME_H

#include <array>

#include <vulkan/vulkan.hpp>

#include "UniformBufferObject.h"
#include "VulkanHelper.h"
#include "scene/Mesh.h"

struct VulkanFrame {
	vk::Device device;
	vk::RenderPass render_pass;
	vk::Extent2D swapchain_extent;

	vk::UniqueImageView image_view;
	vk::UniqueFramebuffer framebuffer;
	vk::UniqueCommandBuffer command_buffer;
	vk::UniqueFence fence;
	vk::UniqueDescriptorSet scene_descriptor_set;
	vk::UniqueDescriptorSet mesh_descriptor_set;

	std::unique_ptr<Buffer> scene_buffer;
	std::unique_ptr<Buffer> transform_buffer;

	void init_frame(vk::Device device);
	void create_framebuffer(vk::Extent2D extent, vk::RenderPass &render_pass, vk::ImageView &depth_image_view);
	void create_command_buffers(vk::CommandPool command_pool);
	void create_sync_objects();
	void create_descriptor_set(vk::DescriptorPool descriptor_pool, vk::DescriptorSetLayout scene_descriptor_set_layout, vk::DescriptorSetLayout mesh_descriptor_set_layout, vk::Sampler texture_sampler, vk::ImageView texture_image_view);
	void create_uniform_buffer(vk::PhysicalDevice physical_device);

	void begin_render_pass();
	void end_render_pass();
};

#endif
