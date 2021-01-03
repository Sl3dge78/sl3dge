#ifndef VULKANAPPLICATION_H
#define VULKANAPPLICATION_H

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <vector>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_vulkan.h"

#include "Debug.h"
#include "Input.h"
#include "Swapchain.h"
#include "UniformBufferObject.h"
#include "Vertex.h"
#include "VulkanFrame.h"
#include "VulkanHelper.h"

const Uint32 WINDOW_WIDTH = 1280;
const Uint32 WINDOW_HEIGHT = 720;
const Uint32 FRAME_RATE = 60;

class VulkanApplication {
public:
	VulkanApplication();
	~VulkanApplication();

	void run();

protected:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VtxUniformBufferObject vubo;
	FragUniformBufferObject fubo;

	SDL_Window *window = nullptr;

	float get_aspect_ratio() const;

private:
	vk::DynamicLoader dynamic_loader;
	vk::UniqueInstance instance;
	vk::UniqueSurfaceKHR surface;
	vk::PhysicalDevice physical_device;
	vk::UniqueDevice device;

	vk::UniqueDebugUtilsMessengerEXT debug_messenger;

	vk::UniqueCommandPool graphics_command_pool;
	vk::UniqueCommandPool transfer_command_pool;

	QueueFamilyIndices queue_family_indices;
	vk::Queue graphics_queue;
	vk::Queue present_queue;

	size_t semaphore_index = 0;

	vk::Queue transfer_queue;

	std::unique_ptr<Buffer> mesh_buffer;

	uint32_t idx_offset;

	std::unique_ptr<Image> texture;
	vk::UniqueSampler texture_sampler;

	UniqueSwapchain swapchain;

	bool framebuffer_rezised = false;
	bool wait_for_vsync = false;

	// Init & Cleanup
	void init_vulkan();

	// Loop
	virtual void load(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) = 0;
	virtual void start() = 0;
	virtual void update(float delta_time) = 0;
	void main_loop();
	void draw_scene(VulkanFrame &frame);
	void draw_frame();

	// Device
	void create_instance();
	void pick_physical_device();
	void create_logical_device();
	void create_mesh_buffer();
	void create_texture_image();
	void create_texture_sampler();

	void copy_buffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
};

#endif
