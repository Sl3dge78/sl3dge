#ifndef VULKANHELPER_H
#define VULKANHELPER_H

#include <fstream>
#include <optional>
#include <set>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <SDL/SDL.h>
#include <SDL/SDL_vulkan.h>

#include "Debug.h"

const std::vector<const char *> req_validation_layers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char *> req_device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	VK_KHR_MAINTENANCE3_EXTENSION_NAME,
	VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
};
struct QueueFamilyIndices {
	std::optional<Uint32> graphics_family;
	std::optional<Uint32> present_family;
	std::optional<Uint32> transfer_family;

	bool is_complete() {
		return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value();
	}
};
class Image {
private:
	vk::Device device;

public:
	vk::Image image;
	vk::DeviceMemory memory;
	vk::ImageView image_view;

	Image(vk::Device device, vk::PhysicalDevice physical_device, const uint32_t w, const uint32_t h, const vk::Format fmt, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage, const vk::MemoryPropertyFlags properties, const vk::ImageAspectFlagBits aspect);
	~Image();
	void transition_layout(vk::CommandBuffer c_buffer, vk::ImageLayout from, vk::ImageLayout to, const uint32_t transfer_family, const uint32_t graphics_family);
};
class Buffer {
private:
	vk::Device device;

public:
	vk::Buffer buffer;
	vk::DeviceMemory memory;

	Buffer(vk::Device device, vk::PhysicalDevice physical_device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
	~Buffer();
	void write_data(void *data, vk::DeviceSize size, const uint32_t offset = 0);
};

bool check_validation_layer_support();
std::vector<const char *> get_required_extensions(SDL_Window *window);

vk::Format find_supported_format(vk::PhysicalDevice physical_device, const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
QueueFamilyIndices find_queue_families(vk::PhysicalDevice device, vk::SurfaceKHR surface);
void check_vk_result(VkResult err);
vk::Format get_vk_format(SDL_PixelFormat *format);

uint32_t find_memory_type(vk::PhysicalDevice physical_device, uint32_t type_filter, vk::MemoryPropertyFlags properties);
void create_buffer(vk::Device device, vk::PhysicalDevice physical_device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer &buffer, vk::UniqueDeviceMemory &memory);
vk::UniqueImage create_image(vk::Device device, vk::Format format, const uint32_t w, const uint32_t h, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage);
vk::UniqueImageView create_image_view(vk::Device device, vk::Image &image, vk::Format format, vk::ImageAspectFlags aspect);
vk::UniqueDeviceMemory create_memory(vk::Device device, vk::PhysicalDevice physical_device, vk::MemoryRequirements reqs, const vk::MemoryPropertyFlags properties);
void copy_buffer_to_image(vk::CommandBuffer c_buffer, vk::Buffer buffer, vk::Image image, const uint32_t w, const uint32_t h);

std::vector<char> read_file(const std::string &path);
#endif
