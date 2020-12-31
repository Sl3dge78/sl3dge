#pragma once

#include <iostream>

#include "VulkanHelper.h"

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
/*
class DebugMessenger {
public:
	DebugMessenger(vk::UniqueInstance instance) {
		debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debug_create_info.pfnUserCallback = debug_callback;
		debug_create_info.pUserData = nullptr;

		instance->createDebugUtilsMessengerEXTUnique()

				if (CreateDebugUtilsMessengerEXT(instance, &debug_create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
			throw std::runtime_error("Unable to create debug msnger");
		}
	}
	~DebugMessenger() {
		DestroyDebugUtilsMessengerEXT(instance_, debug_messenger, nullptr);
	}

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info;

private:
	VkInstance instance_;
	VkDebugUtilsMessengerEXT debug_messenger;

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}
};
*/
