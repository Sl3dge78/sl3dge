#include "VulkanFrame.h"

#include <array>
#include <exception>

#include "UniformBufferObject.h"
#include "VulkanHelper.h"

void VulkanFrame::init_frame(VkDevice device) {
	this->device = device;
}

void VulkanFrame::create_framebuffer(VkExtent2D swapchain_extent, VkRenderPass &render_pass, VkImageView &depth_image_view) {
	VkImageView attachments[] = {
		image_view,
		depth_image_view,
	};

	VkFramebufferCreateInfo framebuffer_ci{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = render_pass,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.width = swapchain_extent.width,
		.height = swapchain_extent.height,
		.layers = 1,
	};

	if (vkCreateFramebuffer(device, &framebuffer_ci, nullptr, &framebuffer) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create Framebuffer!");
	}
}

void VulkanFrame::create_command_buffers(VkCommandPool command_pool) {
	this->command_pool = command_pool;
	VkCommandBufferAllocateInfo ci{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	check_vk_result(vkAllocateCommandBuffers(device, &ci, &command_buffer));
}

void VulkanFrame::create_sync_objects() {
	VkFenceCreateInfo fence_ci{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	vkCreateFence(device, &fence_ci, nullptr, &fence);
}

void VulkanFrame::create_descriptor_set(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, VkSampler texture_sampler, VkImageView texture_image_view) {
	VkDescriptorSetAllocateInfo ai{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptor_set_layout,
	};
	check_vk_result(vkAllocateDescriptorSets(device, &ai, &descriptor_set));

	VkDescriptorBufferInfo vbi{
		.buffer = uniform_buffer,
		.offset = 0,
		.range = sizeof(VtxUniformBufferObject),
	};

	VkWriteDescriptorSet vtx_uniform_descriptor_write{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptor_set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &vbi,
	};

	VkDescriptorImageInfo ii{
		.sampler = texture_sampler,
		.imageView = texture_image_view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};
	VkWriteDescriptorSet texture_descriptor_write{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptor_set,
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &ii,
	};

	VkDescriptorBufferInfo fbi{
		.buffer = uniform_buffer,
		.offset = sizeof(VtxUniformBufferObject),
		.range = sizeof(FragUniformBufferObject),
	};

	VkWriteDescriptorSet frag_uniform_descriptor_write{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptor_set,
		.dstBinding = 2,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &fbi,
	};

	std::array<VkWriteDescriptorSet, 3> descriptor_writes = { vtx_uniform_descriptor_write, texture_descriptor_write, frag_uniform_descriptor_write };
	vkUpdateDescriptorSets(device, uint32_t(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
}

void VulkanFrame::create_uniform_buffer(VkPhysicalDevice physical_device) {
	VkDeviceSize size = sizeof(VtxUniformBufferObject) + sizeof(FragUniformBufferObject);
	create_buffer(device, physical_device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffer, uniform_buffer_memory);
}

void VulkanFrame::delete_frame() {
	vkFreeMemory(device, uniform_buffer_memory, nullptr);
	vkDestroyBuffer(device, uniform_buffer, nullptr);

	vkDestroyFence(device, fence, nullptr);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
	vkDestroyFramebuffer(device, framebuffer, nullptr);
	vkDestroyImageView(device, image_view, nullptr);
}
