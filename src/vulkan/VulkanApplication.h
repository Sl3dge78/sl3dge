#ifndef VULKANAPPLICATION_H
#define VULKANAPPLICATION_H

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <vector>

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
#include "scene/Scene.h"
#include "vulkan/VulkanFrame.h"
#include "vulkan/VulkanHelper.h"
#include "vulkan/VulkanPipeline.h"

#include "Input.h"

const Uint32 WINDOW_WIDTH = 1280;
const Uint32 WINDOW_HEIGHT = 720;
const Uint32 FRAME_RATE = 60;

const std::vector<const char *> req_validation_layers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char *> req_device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	VK_KHR_MAINTENANCE3_EXTENSION_NAME,
	VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
	VK_KHR_RAY_QUERY_EXTENSION_NAME,
	VK_KHR_SHADER_CLOCK_EXTENSION_NAME
};

struct FrameSemaphores {
	vk::UniqueSemaphore image_aquired;
	vk::UniqueSemaphore render_complete;
};

class VulkanApplication {
public:
	VulkanApplication();
	~VulkanApplication();

	void run();

	///Copy a buffer from a staging buffer in the transfer queue to a buffer in the graphics queue transferring ownership.
	void copy_buffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
	vk::CommandBuffer create_commandbuffer(vk::QueueFlagBits type = vk::QueueFlagBits::eGraphics, bool begin = true);
	std::vector<vk::CommandBuffer> create_commandbuffers(uint32_t count, vk::QueueFlagBits type = vk::QueueFlagBits::eGraphics, bool begin = true);
	void flush_commandbuffer(vk::CommandBuffer cmd, vk::QueueFlagBits type = vk::QueueFlagBits::eGraphics, bool wait = true);
	void flush_commandbuffers(std::vector<vk::CommandBuffer> cmd, vk::QueueFlagBits type = vk::QueueFlagBits::eGraphics, bool wait = true);

	vk::Device get_device() { return *device; }
	vk::PhysicalDevice get_physical_device() { return physical_device; }
	int get_graphics_family_index() const;
	int get_transfer_family_index() const;
	float get_aspect_ratio() const;

protected:
	SDL_Window *window = nullptr;
	std::unique_ptr<Scene> scene;

	void refresh_shaders();
	void toggle_rtx();

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
	vk::Queue transfer_queue;

	vk::UniqueSwapchainKHR swapchain;
	uint32_t swapchain_image_count;
	std::vector<vk::Image> swapchain_images;
	vk::Format swapchain_format;
	vk::Extent2D swapchain_extent;

	std::unique_ptr<Image> depth_image;

	std::vector<VulkanFrame> frames;
	std::vector<FrameSemaphores> frame_semaphores;
	size_t semaphore_index = 0;

	bool framebuffer_rezised = false;
	bool wait_for_vsync = false;

	vk::UniqueDescriptorPool imgui_descriptor_pool;
	vk::UniqueRenderPass raster_render_pass;
	vk::UniqueRenderPass ui_render_pass;

	std::unique_ptr<GraphicsPipeline> raster_pipe;

	// RTX
	bool rtx = true;
	vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtx_properties;

	std::unique_ptr<RaytracingPipeline> raytracing_pipeline;

	std::unique_ptr<Image> rtx_result_image;
	std::unique_ptr<Buffer> shader_binding_table;

	void init_vulkan();
	void post_swapchain_init(bool update = false);

	virtual void load() = 0;
	virtual void start() = 0;
	virtual void update(float delta_time) = 0;

	void main_loop();
	void raster_scene(VulkanFrame &frame);
	void draw_ui(VulkanFrame &frame);
	void draw_frame();

	void create_instance();
	void pick_physical_device();
	void create_logical_device();
	void create_texture_sampler();

	void create_swapchain();

	void build_raster_pipeline(bool update = false);
	void create_raster_renderpass();

	void create_ui_context();
	void create_frames();
	void create_semaphores();
	void create_ui_renderpass();

	void init_rtx();
	void build_rtx_pipeline(bool update = false);
	void create_rtx_SBT();
	void raytrace(VulkanFrame &frame, int image_id);

	bool check_validation_layer_support();
	std::vector<const char *> get_required_extensions(SDL_Window *window);
	vk::Format find_depth_format();
	vk::SurfaceFormatKHR choose_surface_format(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);
	vk::PresentModeKHR choose_present_mode(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);
	vk::Extent2D choose_extent(const vk::SurfaceCapabilitiesKHR &capabilities, SDL_Window *window);
};

#endif
