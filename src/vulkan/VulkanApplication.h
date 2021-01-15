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

#include "Swapchain.h"
#include "VulkanFrame.h"
#include "VulkanHelper.h"

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
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
};

class VulkanApplication {
public:
	VulkanApplication();
	~VulkanApplication();

	void run();
	vk::Device get_device() { return *device; }
	vk::PhysicalDevice get_physical_device() { return physical_device; }
	void copy_buffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
	vk::CommandBuffer create_commandbuffer(vk::QueueFlagBits type = vk::QueueFlagBits::eGraphics, bool begin = true);
	std::vector<vk::CommandBuffer> create_commandbuffers(uint32_t count, vk::QueueFlagBits type = vk::QueueFlagBits::eGraphics, bool begin = true);
	void flush_commandbuffer(vk::CommandBuffer cmd, vk::QueueFlagBits type = vk::QueueFlagBits::eGraphics, bool wait = true);
	void flush_commandbuffers(std::vector<vk::CommandBuffer> cmd, vk::QueueFlagBits type = vk::QueueFlagBits::eGraphics, bool wait = true);

protected:
	SDL_Window *window = nullptr;
	std::unique_ptr<Scene> scene;

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
	vk::Queue transfer_queue;

	std::unique_ptr<Image> texture;
	vk::UniqueSampler texture_sampler;

	UniqueSwapchain swapchain;
	size_t semaphore_index = 0;

	bool framebuffer_rezised = false;
	bool wait_for_vsync = false;

	// Init & Cleanup
	void init_vulkan();

	// Loop
	virtual void load() = 0;
	virtual void start() = 0;
	virtual void update(float delta_time) = 0;
	void main_loop();
	void rasterize_scene(VulkanFrame &frame);
	void draw_ui(VulkanFrame &frame);
	void draw_frame();

	void create_instance();
	void pick_physical_device();
	void create_logical_device();
	void create_texture_image();
	void create_texture_sampler();

	bool check_validation_layer_support();
	std::vector<const char *> get_required_extensions(SDL_Window *window);

	// RTX
protected:
	struct RTPushConstant {
		glm::vec4 clear_color;
		glm::vec3 light_pos;
		float light_intensity;
		int light_type;
	} rtx_push_constants;

private:
	vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtx_properties;

	vk::UniqueDescriptorPool rtx_pool;
	vk::UniqueDescriptorSetLayout rtx_set_layout;
	vk::UniqueDescriptorSet rtx_set;
	vk::UniquePipelineLayout rtx_layout;
	vk::UniquePipeline rtx_pipeline;

	std::unique_ptr<Image> rtx_result_image;
	std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups;
	std::unique_ptr<Buffer> shader_binding_table;

	void init_rtx();
	void build_rtx_pipeline();
	void create_rtx_SBT();
	void raytrace(VulkanFrame &frame, int image_id);
};

#endif
