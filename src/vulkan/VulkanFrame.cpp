#include "VulkanFrame.h"

#include "scene/Scene.h"
#include "vulkan/VulkanHelper.h"

void VulkanFrame::init_frame(vk::Device device) {
	this->device = device;
}
void VulkanFrame::create_framebuffer(vk::Extent2D extent, vk::RenderPass &render_pass_, vk::ImageView depth_imageview) {
	render_pass = render_pass_;
	swapchain_extent = extent;

	std::vector<vk::ImageView> attachments;
	attachments.push_back(*raster_image_view);

	if (depth_imageview) {
		attachments.push_back(depth_imageview);
	}

	framebuffer = device.createFramebufferUnique(vk::FramebufferCreateInfo({}, render_pass, attachments, swapchain_extent.width, swapchain_extent.height, 1));
}
void VulkanFrame::create_command_buffers(vk::CommandPool command_pool) {
	command_buffer = std::move(device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(command_pool, vk::CommandBufferLevel::ePrimary, 1)).front());
}
void VulkanFrame::create_sync_objects() {
	fence = device.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
}
void VulkanFrame::begin_render_pass() {
	vk::Rect2D rect(vk::Offset2D(0, 0), swapchain_extent);
	std::array<vk::ClearValue, 2> clearValues;
	clearValues[0].color.setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f });
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;
	command_buffer->beginRenderPass(vk::RenderPassBeginInfo(render_pass, *framebuffer, rect, clearValues), vk::SubpassContents::eInline);
}
void VulkanFrame::end_render_pass() {
	command_buffer->endRenderPass();
}
