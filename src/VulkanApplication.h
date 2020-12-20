#ifndef VULKANAPPLICATION_H
#define VULKANAPPLICATION_H

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "imgui/imgui.h"

#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_vulkan.h"

#include "Input.h"
#include "Debug.h"

const Uint32 WINDOW_WIDTH = 1280;
const Uint32 WINDOW_HEIGHT = 720;
const Uint32 FRAME_RATE = 60;
const Uint32 MAX_FRAMES_IN_FLIGHT = 2;
const std::vector<const char *> validation_layers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char *> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifndef _DEBUG
const bool enable_validation_layers = false;
#else

const bool enable_validation_layers = true;
#endif

struct QueueFamilyIndices {
	std::optional<Uint32> graphics_family;
	std::optional<Uint32> present_family;
	std::optional<Uint32> transfer_family;

	bool is_complete() {
		return graphics_family.has_value() && present_family.has_value() && transfer_family.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 tex_coord;

	bool operator==(const Vertex &other) const {
		return pos == other.pos && normal == other.normal && tex_coord == other.tex_coord;
	}

	static VkVertexInputBindingDescription get_binding_description() {
		VkVertexInputBindingDescription description{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};

		return description;
	}

	static std::array<VkVertexInputAttributeDescription, 3> get_attribute_descriptions() {
		std::array<VkVertexInputAttributeDescription, 3> descriptions{};
		descriptions[0].location = 0;
		descriptions[0].binding = 0;
		descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		descriptions[0].offset = offsetof(Vertex, pos);

		descriptions[1].location = 1;
		descriptions[1].binding = 0;
		descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		descriptions[1].offset = offsetof(Vertex, normal);

		descriptions[2].location = 2;
		descriptions[2].binding = 0;
		descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		descriptions[2].offset = offsetof(Vertex, tex_coord);

		return descriptions;
	}
};
namespace std {
template <>
struct hash<Vertex> {
	size_t operator()(Vertex const &vertex) const {
		return ((hash<glm::vec3>()(vertex.pos) ^
						(hash<glm::vec3>()(vertex.normal) << 1)) >>
					   1) ^
			   (hash<glm::vec2>()(vertex.tex_coord) << 1);
	}
};
} // namespace std

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 view_position;
};

struct VulkanFrame {
	VkDevice device;
	VkImage image;
	VkImageView image_view;

	VkFramebuffer framebuffer;

	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;

	VkFence fence;

	VkSemaphore semaphore_image_available;
	VkSemaphore semaphore_render_finished;
	VkFence inflight_fence;

	VkDescriptorSet descriptor_set;
	VkBuffer uniform_buffer;
	VkDeviceMemory uniform_buffer_memory;

	void init_frame(VkDevice device); 
	void create_framebuffer(VkExtent2D swapchain_extent, VkRenderPass render_pass, VkImageView depth_image_view); 
	void create_command_buffers(VkCommandPool command_pool); 
	void create_sync_objects(); 
	void create_descriptor_set(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, VkSampler texture_sampler, VkImageView texture_image_view);
	void create_uniform_buffer(VkPhysicalDevice physical_device); 
	void delete_frame();
};

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
	UniformBufferObject transform;

	VkExtent2D swapchain_extent;
	SDL_Window *window = nullptr;

private:
	VkInstance instance;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice device;

	QueueFamilyIndices queue_family_indices;
	VkQueue graphics_queue;
	VkQueue present_queue;

	VkSwapchainKHR swapchain;
	VkFormat swapchain_format;
	std::vector<VulkanFrame> frames;

	VkRenderPass render_pass;
	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;

	VkCommandPool graphics_command_pool;

	std::vector<FrameSemaphores> frame_semaphores;
	size_t semaphore_index = 0;

	VkQueue transfer_queue;
	VkCommandPool transfer_command_pool;

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

	VkDebugUtilsMessengerEXT debug_messenger;
	VkSurfaceKHR surface;

	VkDescriptorPool imgui_descriptor_pool;
	bool framebuffer_rezised = false;
	bool wait_for_vsync = false;

	void init_window();
	void init_imgui();
	void create_imgui_context();
	void cleanup_imgui_context();
	void init_vulkan();
	void main_loop();
	virtual void load(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) = 0;
	virtual void start() = 0;
	virtual void update(float delta_time) = 0;
	void cleanup();

	void draw_ui(VulkanFrame &frame);
	void draw_scene(VulkanFrame &frame);
	void draw_frame();

	// Device
	void create_instance();
	void setup_debug_messenger();
	void create_surface();
	void pick_physical_device();
	void create_logical_device();

	// Swapchain
	void init_swapchain(bool reset = false);
	void create_swapchain();
	void cleanup_swapchain();

	// Rendering
	void create_image_views();
	void create_render_pass();
	void create_graphics_pipeline();
	void create_framebuffers();
	void create_command_pools();
	void create_command_buffers();
	void create_sync_objects();

	// Scene
	void create_mesh_buffer();
	void create_uniform_buffer();
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
	VkFormat find_supported_format(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat find_depth_format();
	bool has_stencil_component(VkFormat format);

	void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

	std::vector<const char *> get_required_extensions();

	bool check_validation_layer_support();

	bool is_device_suitable(VkPhysicalDevice device);
	bool check_device_extension_support(VkPhysicalDevice device);
	QueueFamilyIndices find_queue_families(VkPhysicalDevice device);

	VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats);
	VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> &available_present_modes);
	VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR &capabilities);
	SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);

	VkShaderModule create_shader_module(const std::vector<char> &code);

	// Cmd
	void create_command_pool(Uint32 queue_family_index, VkCommandPoolCreateFlags flags, VkCommandPool *command_pool);
	void create_command_buffer(VkCommandPool pool, VkCommandBuffer *c_buffer);
	VkCommandBuffer begin_graphics_command_buffer();
	void end_graphics_command_buffer(VkCommandBuffer c_buffer);
	VkCommandBuffer begin_transfer_command_buffer();
	void end_transfer_command_buffer(VkCommandBuffer c_buffer);
};

#endif
