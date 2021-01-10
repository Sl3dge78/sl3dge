#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <vulkan/vulkan.hpp>

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
	std::cerr << pCallbackData->pMessage << std::endl
			  << std::endl;

	return VK_FALSE;
}

void debug_set_object_name(vk::Device device, uint64_t object, vk::DebugReportObjectTypeEXT obj_type, const char *name);
#endif