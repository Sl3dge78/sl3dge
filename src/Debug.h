#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <vulkan/vulkan.hpp>

#ifndef _DEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
	std::cerr << pCallbackData->pMessage << std::endl
			  << std::endl;

	return VK_FALSE;
}
void debug_begin_label(vk::CommandBuffer cmd_buf, const std::string label, std::array<float, 4> color = { 1.f, 1.f, 1.f, 1.f });
void debug_end_label(vk::CommandBuffer cmd_buf);
void debug_name_object(vk::Device device, const uint64_t handle, vk::ObjectType type, const char *name);
#endif
