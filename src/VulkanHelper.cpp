#include "VulkanHelper.h"

#include <SDL/SDL_vulkan.h>

std::vector<char> read_file(const std::string &path) {
	std::ifstream file(path, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		SDL_LogError(0, "Failed to open file %s", path.c_str());
		throw std::runtime_error("Failed to open file");
	}
	size_t file_size = (size_t)file.tellg();
	std::vector<char> buffer(file_size);
	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	return buffer;
}

VkFormat get_vk_format(SDL_PixelFormat *format) {
	switch (format->format) {
		case SDL_PIXELFORMAT_ABGR8888:
			return VK_FORMAT_R8G8B8A8_SRGB;
			break;
		case SDL_PIXELFORMAT_RGBA8888:
			return VK_FORMAT_R8G8B8A8_SRGB;
			break;
		default:
			SDL_LogError(0, "Texture pixel format unsupported!");
			throw std::runtime_error("Unable to create texture");
			break;
	}
}

uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) && ((mem_properties.memoryTypes[i].propertyFlags & properties) == properties)) {
			return i;
		}
	}
	throw std::runtime_error("Unable to find suitable memory type");
}

void create_image_view(VkDevice device, VkImage &image, VkFormat format, VkImageAspectFlags aspect, VkImageView *image_view) {
	VkImageViewCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.image = image;

	create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format = format;

	create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	create_info.subresourceRange.aspectMask = aspect;
	create_info.subresourceRange.baseMipLevel = 0;
	create_info.subresourceRange.levelCount = 1;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &create_info, nullptr, image_view) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create image view!");
	}
}

void create_buffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory) {
	VkBufferCreateInfo buffer_info{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};
	vkCreateBuffer(device, &buffer_info, nullptr, &buffer);

	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(device, buffer, &requirements);

	VkMemoryAllocateInfo alloc_info{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = requirements.size,
		.memoryTypeIndex = find_memory_type(physical_device, requirements.memoryTypeBits, properties),
	};
	if (vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
		throw std::runtime_error("Unable to allocate vertex buffer memory");
	}
	vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

void check_vk_result(VkResult err) {
	if (err != VK_SUCCESS) {
		switch (err) {
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				SDL_LogError(0, "Vulkan Error : VK_ERROR_OUT_OF_HOST_MEMORY");
				break;
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				SDL_LogError(0, "Vulkan Error : VK_ERROR_OUT_OF_DEVICE_MEMORY ");
				break;
			case VK_ERROR_FRAGMENTED_POOL:
				SDL_LogError(0, "Vulkan Error : VK_ERROR_FRAGMENTED_POOL");
				break;
			case VK_ERROR_OUT_OF_POOL_MEMORY:
				SDL_LogError(0, "Vulkan Error : VK_ERROR_OUT_OF_POOL_MEMORY");
				break;
			default:
				SDL_LogError(0, "Unhandled exception : %d", err);
		}
		throw std::runtime_error("Vulkan error");
	}
}

bool check_validation_layer_support() {
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char *layer_name : validation_layers) {
		bool layer_found = false;
		for (const auto &layer_properties : available_layers) {
			if (strcmp(layer_name, layer_properties.layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found) {
			return false;
		}
	}

	return true;
}

std::vector<const char *> get_required_extensions(SDL_Window *window) {
	// Get extension count & names
	uint32_t sdl_extension_count = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, nullptr);

	std::vector<const char *> required_extension_names = {};
	size_t sdl_additional_extension_count = required_extension_names.size();
	required_extension_names.resize(sdl_additional_extension_count + sdl_extension_count);
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, required_extension_names.data() + sdl_additional_extension_count);

	if (enable_validation_layers) {
		required_extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	SDL_Log("Required extensions:");
	for (const auto &extension : required_extension_names) {
		SDL_Log("\t %s", extension);
	}

	return required_extension_names;
}
