#include "VulkanFrame.h"

#include "VulkanHelper.h"

void VulkanFrame::init_frame(vk::Device device) {
	this->device = device;
}

void VulkanFrame::create_framebuffer(vk::Extent2D extent, vk::RenderPass &render_pass_, vk::ImageView &depth_image_view) {
	render_pass = render_pass_;
	swapchain_extent = extent;

	auto attachments = {
		*image_view,
		depth_image_view,
	};

	framebuffer = device.createFramebufferUnique(vk::FramebufferCreateInfo({}, render_pass, attachments, swapchain_extent.width, swapchain_extent.height, 1));
}

void VulkanFrame::create_command_buffers(vk::CommandPool command_pool) {
	command_buffer = std::move(device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(command_pool, vk::CommandBufferLevel::ePrimary, 1)).front());
}

void VulkanFrame::create_sync_objects() {
	fence = device.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
}

void VulkanFrame::create_descriptor_set(vk::DescriptorPool descriptor_pool, vk::DescriptorSetLayout descriptor_set_layout, vk::Sampler texture_sampler, vk::ImageView texture_image_view) {
	auto dldl = device.allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(descriptor_pool, descriptor_set_layout));
	descriptor_set = std::move(dldl.front());

	auto v_bi = vk::DescriptorBufferInfo(*uniform_buffer, 0, sizeof(VtxUniformBufferObject));
	auto f_bi = vk::DescriptorBufferInfo(*uniform_buffer, sizeof(VtxUniformBufferObject), sizeof(FragUniformBufferObject));
	auto image_info = vk::DescriptorImageInfo(texture_sampler, texture_image_view, vk::ImageLayout::eShaderReadOnlyOptimal);

	auto descriptor_writes = {
		vk::WriteDescriptorSet(*descriptor_set, 0, 0, vk::DescriptorType::eUniformBuffer, nullptr, v_bi, nullptr),
		vk::WriteDescriptorSet(*descriptor_set, 1, 0, vk::DescriptorType::eCombinedImageSampler, image_info, nullptr, nullptr),
		vk::WriteDescriptorSet(*descriptor_set, 2, 0, vk::DescriptorType::eUniformBuffer, nullptr, f_bi, nullptr),

	};

	device.updateDescriptorSets(descriptor_writes, nullptr);
}

void VulkanFrame::create_uniform_buffer(vk::PhysicalDevice physical_device) {
	vk::DeviceSize size = sizeof(VtxUniformBufferObject) + sizeof(FragUniformBufferObject);
	create_buffer(device, physical_device, size, vk::BufferUsageFlagBits::eUniformBuffer, { vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent }, uniform_buffer, uniform_buffer_memory);
}
void VulkanFrame::begin_render_pass() {
	command_buffer->begin(vk::CommandBufferBeginInfo({}, nullptr));
	std::array<vk::ClearValue, 2> clear_values{};
	clear_values[0].color = vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f });
	clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.f, 0);
	vk::Rect2D rect(vk::Offset2D(0, 0), swapchain_extent);
	command_buffer->beginRenderPass(vk::RenderPassBeginInfo(render_pass, *framebuffer, rect, clear_values), vk::SubpassContents::eInline);
}
void VulkanFrame::end_render_pass() {
	command_buffer->endRenderPass();
	command_buffer->end();
}
