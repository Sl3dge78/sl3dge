#include "VulkanHelper.h"
#include "VulkanApplication.h"

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
Image::Image(VulkanApplication &app, const uint32_t w, const uint32_t h, const vk::Format fmt, const vk::ImageTiling tiling, const vk::ImageUsageFlags usage, const vk::MemoryPropertyFlags properties, const vk::ImageAspectFlagBits aspect) {
	this->device = app.get_device();
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
	memory = device.allocateMemory(vk::MemoryAllocateInfo(mem_reqs.size, find_memory_type(app.get_physical_device(), mem_reqs.memoryTypeBits, properties)));
	device.bindImageMemory(image, memory, 0);

	image_view = device.createImageView(vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, fmt, vk::ComponentMapping(), vk::ImageSubresourceRange(aspect, 0, 1, 0, 1)));
}
void Image::transition_layout(vk::CommandBuffer c_buffer, vk::ImageLayout from, vk::ImageLayout to, const uint32_t transfer_family, const uint32_t graphics_family) {
	vk::ImageMemoryBarrier barrier({}, {}, from, to, transfer_family, transfer_family, this->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

	vk::PipelineStageFlags src_stage;
	vk::PipelineStageFlags dst_stage;
	if (from == vk::ImageLayout::eUndefined && to == vk::ImageLayout::eTransferDstOptimal) {
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;

		barrier.srcQueueFamilyIndex = transfer_family;
		barrier.dstQueueFamilyIndex = transfer_family;

	} else if (from == vk::ImageLayout::eTransferDstOptimal && to == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = {};

		barrier.srcQueueFamilyIndex = transfer_family;
		barrier.dstQueueFamilyIndex = graphics_family;

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;

	} else {
		throw std::runtime_error("Unsupported layout transition");
	}
	c_buffer.pipelineBarrier(src_stage, dst_stage, {}, nullptr, nullptr, barrier);
}
Image::~Image() {
	device.destroyImage(image);
	device.freeMemory(memory);
	device.destroyImageView(image_view);
}

Buffer::Buffer(vk::Device device, vk::PhysicalDevice physical_device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
	this->device = device;
	buffer = device.createBuffer(vk::BufferCreateInfo({}, size, { usage | vk::BufferUsageFlagBits::eShaderDeviceAddress }, vk::SharingMode::eExclusive, nullptr));
	auto requirements = device.getBufferMemoryRequirements(buffer);
	vk::MemoryAllocateInfo ai(requirements.size, find_memory_type(physical_device, requirements.memoryTypeBits, properties));
	vk::MemoryAllocateFlagsInfo fi(vk::MemoryAllocateFlagBits::eDeviceAddress);
	ai.setPNext(&fi);
	memory = device.allocateMemory(ai);
	device.bindBufferMemory(buffer, memory, 0);
	address = device.getBufferAddress(vk::BufferDeviceAddressInfo(buffer));
}
Buffer::Buffer(VulkanApplication &app, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
	this->device = app.get_device();
	buffer = device.createBuffer(vk::BufferCreateInfo({}, size, { usage | vk::BufferUsageFlagBits::eShaderDeviceAddress }, vk::SharingMode::eExclusive, nullptr));
	auto requirements = device.getBufferMemoryRequirements(buffer);
	vk::MemoryAllocateInfo ai(requirements.size, find_memory_type(app.get_physical_device(), requirements.memoryTypeBits, properties));
	vk::MemoryAllocateFlagsInfo fi(vk::MemoryAllocateFlagBits::eDeviceAddress);
	ai.setPNext(&fi);
	memory = device.allocateMemory(ai);
	device.bindBufferMemory(buffer, memory, 0);
	address = device.getBufferAddress(vk::BufferDeviceAddressInfo(buffer));
}
Buffer::~Buffer() {
	device.destroyBuffer(buffer);
	device.freeMemory(memory);
}
void Buffer::write_data(void *data, vk::DeviceSize size, const uint32_t offset) {
	void *p = device.mapMemory(this->memory, offset, size, {});
	memcpy(p, data, size);
	device.unmapMemory(this->memory);
}

AccelerationStructure::AccelerationStructure(vk::Device device, vk::PhysicalDevice physical_device, vk::AccelerationStructureTypeKHR type, vk::DeviceSize size) {
	buffer = std::unique_ptr<Buffer>(new Buffer(device, physical_device, size, { vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress }, vk::MemoryPropertyFlagBits::eDeviceLocal));
	vk::AccelerationStructureCreateInfoKHR create_info({}, buffer->buffer, 0, size, type, {});

	acceleration_structure = device.createAccelerationStructureKHRUnique(create_info);
	address = device.getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR(*acceleration_structure));
}
AccelerationStructure::AccelerationStructure(VulkanApplication &app, vk::AccelerationStructureTypeKHR type, vk::DeviceSize size) {
	buffer = std::unique_ptr<Buffer>(new Buffer(app, size, { vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress }, vk::MemoryPropertyFlagBits::eDeviceLocal));
	vk::AccelerationStructureCreateInfoKHR create_info({}, buffer->buffer, 0, size, type, {});

	acceleration_structure = app.get_device().createAccelerationStructureKHRUnique(create_info);
	address = app.get_device().getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR(*acceleration_structure));
}
vk::DeviceAddress AccelerationStructure::get_address() {
	return address;
}
const vk::AccelerationStructureKHR AccelerationStructure::get_acceleration_structure() {
	return *acceleration_structure;
}

vk::Format find_supported_format(vk::PhysicalDevice physical_device, const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
	for (vk::Format format : candidates) {
		vk::FormatProperties props = physical_device.getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
			//SDL_Log("Format = %d", format);
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
			return vk::Format::eR8G8B8A8Unorm;

		case SDL_PIXELFORMAT_RGBA8888:
			return vk::Format::eR8G8B8A8Unorm;

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
vk::Buffer create_buffer(vk::Device device, vk::PhysicalDevice physical_device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
	return device.createBuffer(vk::BufferCreateInfo({}, size, usage, vk::SharingMode::eExclusive, nullptr));
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
vk::UniqueImageView create_image_view(vk::Device device, vk::Image &image, vk::Format format, vk::ImageAspectFlags aspect) {
	return std::move(device.createImageViewUnique(vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, format, vk::ComponentMapping(), vk::ImageSubresourceRange(aspect, 0, 1, 0, 1))));
}
vk::UniqueDeviceMemory create_memory(vk::Device device, vk::PhysicalDevice physical_device, vk::MemoryRequirements reqs, const vk::MemoryPropertyFlags properties) {
	return std::move(device.allocateMemoryUnique(vk::MemoryAllocateInfo(reqs.size, find_memory_type(physical_device, reqs.memoryTypeBits, properties))));
}
void copy_buffer_to_image(vk::CommandBuffer c_buffer, vk::Buffer buffer, vk::Image image, const uint32_t w, const uint32_t h) {
	vk::BufferImageCopy region(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0), vk::Extent3D(w, h, 1));
	c_buffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
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
