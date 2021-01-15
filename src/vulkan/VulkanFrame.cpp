#include "VulkanFrame.h"

#include "scene/Scene.h"
#include "vulkan/VulkanHelper.h"

void VulkanFrame::init_frame(vk::Device device) {
	this->device = device;
}
void VulkanFrame::create_framebuffer(vk::Extent2D extent, vk::RenderPass &render_pass_) {
	render_pass = render_pass_;
	swapchain_extent = extent;

	auto attachments = {
		*image_view,
	};

	framebuffer = device.createFramebufferUnique(vk::FramebufferCreateInfo({}, render_pass, attachments, swapchain_extent.width, swapchain_extent.height, 1));
}
void VulkanFrame::create_command_buffers(vk::CommandPool command_pool) {
	command_buffer = std::move(device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(command_pool, vk::CommandBufferLevel::ePrimary, 1)).front());
}
void VulkanFrame::create_sync_objects() {
	fence = device.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
}
/*
void VulkanFrame::create_descriptor_set(vk::DescriptorPool descriptor_pool, vk::DescriptorSetLayout scene_descriptor_set_layout, vk::DescriptorSetLayout mesh_descriptor_set_layout, vk::Sampler texture_sampler, vk::ImageView texture_image_view) {
	auto image_info = vk::DescriptorImageInfo(texture_sampler, texture_image_view, vk::ImageLayout::eShaderReadOnlyOptimal);
	auto descriptor_writes = {
		vk::WriteDescriptorSet(*scene_descriptor_set, 1, 0, vk::DescriptorType::eCombinedImageSampler, image_info, nullptr, nullptr),
	};
	device.updateDescriptorSets(descriptor_writes, nullptr);
	
}
*/
void VulkanFrame::begin_render_pass() {
	vk::Rect2D rect(vk::Offset2D(0, 0), swapchain_extent);
	vk::ClearValue clear(vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f }));
	command_buffer->beginRenderPass(vk::RenderPassBeginInfo(render_pass, *framebuffer, rect, clear), vk::SubpassContents::eInline);
}
void VulkanFrame::end_render_pass() {
	command_buffer->endRenderPass();
}
