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
void VulkanFrame::begin_render_pass() {
	vk::Rect2D rect(vk::Offset2D(0, 0), swapchain_extent);
	vk::ClearValue clear(vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f }));
	command_buffer->beginRenderPass(vk::RenderPassBeginInfo(render_pass, *framebuffer, rect, clear), vk::SubpassContents::eInline);
}
void VulkanFrame::end_render_pass() {
	command_buffer->endRenderPass();
}
