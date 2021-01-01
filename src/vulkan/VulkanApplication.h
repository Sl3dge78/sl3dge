#pragma once

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
#include "vulkan/UniformBufferObject.h"
#include "vulkan/Vertex.h"
#include "vulkan/VulkanFrame.h"
#include "vulkan/VulkanHelper.h"

const Uint32 WINDOW_WIDTH = 1280;
const Uint32 WINDOW_HEIGHT = 720;
const Uint32 FRAME_RATE = 60;
const Uint32 MAX_FRAMES_IN_FLIGHT = 2;

struct FrameSemaphores {
	VkSemaphore image_aquired;
	VkSemaphore render_complete;
};

class VulkanApplication {
public:
	void run() {
		init_window();
		init_imgui();
		init_vulkan();
		load(vertices, indices);
		create_mesh_buffer();
		main_loop();
		cleanup();
	}

protected:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VtxUniformBufferObject vubo;
	FragUniformBufferObject fubo;

	vk::Extent2D swapchain_extent;
	SDL_Window *window = nullptr;

private:
	vk::DynamicLoader dynamic_loader;
	vk::UniqueInstance instance;
	vk::PhysicalDevice physical_device;
	vk::UniqueDevice device;

	vk::UniqueDebugUtilsMessengerEXT debug_messenger;

	vk::UniqueCommandPool graphics_command_pool;
	vk::UniqueCommandPool transfer_command_pool;

	QueueFamilyIndices queue_family_indices;
	VkQueue graphics_queue;
	VkQueue present_queue;

	vk::UniqueSwapchainKHR swapchain;
	vk::Format swapchain_format;

	std::vector<VulkanFrame> frames;

	VkRenderPass render_pass;
	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;

	std::vector<FrameSemaphores> frame_semaphores;
	size_t semaphore_index = 0;

	VkQueue transfer_queue;

	VkBuffer mesh_buffer;
	VkDeviceMemory mesh_buffer_memory;
	uint32_t idx_offset;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSetLayout descriptor_set_layout;

	VkImage texture_image;
	VkDeviceMemory texture_image_memory;
	VkImageView texture_image_view;
	VkSampler texture_sampler;

	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;

	vk::SurfaceKHR surface;

	VkDescriptorPool imgui_descriptor_pool;

	bool framebuffer_rezised = false;
	bool wait_for_vsync = false;

	// Init & Cleanup
	void init_window();
	void init_vulkan();
	void cleanup();

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

	// Swapchain
	void init_swapchain(bool reset = false);
	void create_swapchain();
	void cleanup_swapchain();

	// Rendering
	void create_render_pass();
	void create_graphics_pipeline();
	void create_sync_objects();

	// Scene
	void create_mesh_buffer();
	void create_descriptors();

	// Texture
	void create_texture_image();
	void create_image(uint32_t w, uint32_t h, VkFormat fmt, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
	void transition_image_layout(VkCommandBuffer c_buffer, VkImage image, VkFormat format, VkImageLayout from, VkImageLayout to);
	void copy_buffer_to_image(VkCommandBuffer transfer_cbuffer, VkBuffer buffer, VkImage image, uint32_t w, uint32_t h);
	void create_texture_image_view();
	void create_texture_sampler();

	// Depth
	void create_depth_resources();
	vk::Format find_supported_format(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
	vk::Format find_depth_format();
	bool has_stencil_component(VkFormat format);

	// IMGUI
	void init_imgui();
	void cleanup_imgui();
	void create_imgui_context();
	void cleanup_imgui_context();
	void draw_ui(VulkanFrame &frame);

	void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

	// Cmd
	void create_command_buffer(VkCommandPool pool, VkCommandBuffer *c_buffer);
	VkCommandBuffer begin_graphics_command_buffer();
	void end_graphics_command_buffer(VkCommandBuffer c_buffer);
	VkCommandBuffer begin_transfer_command_buffer();
	void end_transfer_command_buffer(VkCommandBuffer c_buffer);
};
