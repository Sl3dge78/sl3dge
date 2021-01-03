#include "VulkanHelper.h"

Image::Image(vk::Device device, vk::PhysicalDevice physical_device, const uint32_t w, const uint32_t h, const vk::Format fmt, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage, const vk::MemoryPropertyFlags properties, const vk::ImageAspectFlagBits aspect) {
	this->device = device;
	image = device.createImage(vk::ImageCreateInfo(
			{}, vk::ImageType::e2D,
			fmt, vk::Extent3D(w, h, 1),
			1,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling(tiling),
			vk::ImageUsageFlags(usage),
			vk::SharingMode::eExclusive,
			nullptr,
			vk::ImageLayout::eUndefined));

	// Allocate VkImage memory
	auto mem_reqs = device.getImageMemoryRequirements(image);
	memory = device.allocateMemory(vk::MemoryAllocateInfo(mem_reqs.size, find_memory_type(physical_device, mem_reqs.memoryTypeBits, properties)));
	device.bindImageMemory(image, memory, 0);

	image_view = device.createImageView(vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, fmt, vk::ComponentMapping(), vk::ImageSubresourceRange(aspect, 0, 1, 0, 1)));
}
Image::~Image() {
	device.destroyImage(image);
	device.freeMemory(memory);
	device.destroyImageView(image_view);
}

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
vk::Format get_vk_format(SDL_PixelFormat *format) {
	switch (format->format) {
		case SDL_PIXELFORMAT_ABGR8888:
			return vk::Format::eR8G8B8A8Srgb;

		case SDL_PIXELFORMAT_RGBA8888:
			return vk::Format::eR8G8B8A8Srgb;

		default:
			SDL_LogError(0, "Texture pixel format unsupported!");
			throw std::runtime_error("Unable to create texture");
	}
}
uint32_t find_memory_type(vk::PhysicalDevice physical_device, uint32_t type_filter, vk::MemoryPropertyFlags properties) {
	auto mem_properties = physical_device.getMemoryProperties();
	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) && ((mem_properties.memoryTypes[i].propertyFlags & properties) == properties)) {
			return i;
		}
	}
	throw std::runtime_error("Unable to find suitable memory type");
}
vk::UniqueImageView create_image_view(vk::Device device, vk::Image &image, vk::Format format, vk::ImageAspectFlags aspect) {
	return std::move(device.createImageViewUnique(vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, format, vk::ComponentMapping(), vk::ImageSubresourceRange(aspect, 0, 1, 0, 1))));
}
void create_buffer(vk::Device device, vk::PhysicalDevice physical_device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer &buffer, vk::UniqueDeviceMemory &memory) {
	buffer = std::move(device.createBufferUnique(vk::BufferCreateInfo({}, size, usage, vk::SharingMode::eExclusive, nullptr)));
	auto requirements = device.getBufferMemoryRequirements(*buffer);
	memory = std::move(device.allocateMemoryUnique(vk::MemoryAllocateInfo(requirements.size, find_memory_type(physical_device, requirements.memoryTypeBits, properties))));
	device.bindBufferMemory(*buffer, *memory, 0);
}
void create_image__(vk::Device device, vk::PhysicalDevice physical_device, const uint32_t w, const uint32_t h, const vk::Format fmt, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage, const vk::MemoryPropertyFlags properties, vk::UniqueImage &image, vk::UniqueDeviceMemory &image_memory) {
	// Create VkImage
	image = std::move(device.createImageUnique(vk::ImageCreateInfo(
			{}, vk::ImageType::e2D,
			fmt, vk::Extent3D(w, h, 1),
			1,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling(tiling),
			vk::ImageUsageFlags(usage),
			vk::SharingMode::eExclusive,
			nullptr,
			vk::ImageLayout::eUndefined)));

	// Allocate VkImage memory
	auto mem_reqs = device.getImageMemoryRequirements(*image);
	image_memory = std::move(device.allocateMemoryUnique(vk::MemoryAllocateInfo(mem_reqs.size, find_memory_type(physical_device, mem_reqs.memoryTypeBits, properties))));
	device.bindImageMemory(*image, *image_memory, 0);
}
vk::UniqueImage create_image(vk::Device device, vk::Format format, const uint32_t w, const uint32_t h, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage) {
	return std::move(device.createImageUnique(vk::ImageCreateInfo(
			{}, vk::ImageType::e2D,
			format, vk::Extent3D(w, h, 1),
			1,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling(tiling),
			vk::ImageUsageFlags(usage),
			vk::SharingMode::eExclusive,
			nullptr,
			vk::ImageLayout::eUndefined)));
}
vk::UniqueDeviceMemory create_memory(vk::Device device, vk::PhysicalDevice physical_device, vk::MemoryRequirements reqs, const vk::MemoryPropertyFlags properties) {
	return std::move(device.allocateMemoryUnique(vk::MemoryAllocateInfo(reqs.size, find_memory_type(physical_device, reqs.memoryTypeBits, properties))));
}
bool check_validation_layer_support() {
	auto available_layers = vk::enumerateInstanceLayerProperties();

	for (const char *layer_name : req_validation_layers) {
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
	required_extension_names.resize(sdl_extension_count);
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, required_extension_names.data());

	if (enable_validation_layers) {
		required_extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	SDL_Log("Required extensions:");
	for (const auto &extension : required_extension_names) {
		SDL_Log("\t %s", extension);
	}

	return required_extension_names;
}
vk::SurfaceFormatKHR choose_swapchain_surface_format(const vk::PhysicalDevice physical_device, const vk::SurfaceKHR surface) {
	for (const auto &format : physical_device.getSurfaceFormatsKHR(surface)) {
		if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
			return format;
		}
	}
	return physical_device.getSurfaceFormatsKHR(surface).front();
}
vk::PresentModeKHR choose_swapchain_present_mode(const vk::PhysicalDevice physical_device, const vk::SurfaceKHR surface) {
	for (const auto &mode : physical_device.getSurfacePresentModesKHR(surface)) {
		if (mode == vk::PresentModeKHR::eMailbox) {
			return mode;
		}
	}
	SDL_Log("Present mode: VK_PRESENT_MODE_FIFO_KHR");
	return vk::PresentModeKHR::eFifo;
}
vk::Extent2D choose_swapchain_extent(const vk::SurfaceCapabilitiesKHR &capabilities, SDL_Window *window) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		int w, h;
		SDL_Vulkan_GetDrawableSize(window, &w, &h);
		vk::Extent2D extent = { uint32_t(w), uint32_t(h) };
		extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
		extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));
		return extent;
	}
}
vk::Format find_depth_format(vk::PhysicalDevice physical_device) {
	return find_supported_format(physical_device,
			{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}
vk::Format find_supported_format(vk::PhysicalDevice physical_device, const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
	for (vk::Format format : candidates) {
		vk::FormatProperties props = physical_device.getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}
QueueFamilyIndices find_queue_families(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
	QueueFamilyIndices indices;
	auto queue_family_properties = device.getQueueFamilyProperties();

	int i = 0;
	for (const auto &queue_family : queue_family_properties) {
		if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphics_family = i;
		}

		if (device.getSurfaceSupportKHR(i, surface)) {
			indices.present_family = i;
		}

		if ((queue_family.queueFlags & vk::QueueFlagBits::eTransfer) && !(queue_family.queueFlags & vk::QueueFlagBits::eGraphics)) {
			indices.transfer_family = i;
		}

		if (indices.is_complete()) {
			break;
		}

		i++;
	}
	return indices;
}
