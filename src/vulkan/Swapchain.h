#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include <SDL/SDL.h>
#include <vulkan/vulkan.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"

#include "VulkanFrame.h"
#include "VulkanHelper.h"
#include "scene/Vertex.h"

struct FrameSemaphores {
	vk::UniqueSemaphore image_aquired;
	vk::UniqueSemaphore render_complete;
};

struct SwapchainCreateInfo {
	vk::Instance instance;
	vk::SurfaceKHR surface;
	SDL_Window *window;
	vk::PhysicalDevice physical_device;
	vk::Device device;
	uint32_t queue_family;
	vk::Queue queue;
	vk::CommandPool command_pool;
	vk::Sampler texture_sampler;
	vk::ImageView texture_image_view;
};

class Swapchain {
public:
	Swapchain();
	~Swapchain();

	static SwapchainCreateInfo info; // TODO : c'est degueu ca
	static const bool log = false;

	vk::Semaphore get_image_acquired_semaphore(int id) const { return frame_semaphores[id].image_aquired.get(); }
	vk::Semaphore get_render_complete_semaphore(int id) const { return frame_semaphores[id].render_complete.get(); }
	vk::SwapchainKHR get_swapchain() { return swapchain.get(); }
	vk::SwapchainKHR *get_swapchain_ptr() { return &swapchain.get(); }
	VulkanFrame *get_frame(int id) { return &frames[id]; }
	const uint32_t get_image_count() const { return image_count; }
	vk::Extent2D get_extent() { return extent; }
	vk::Format get_format() { return format; }
	vk::Image get_image(int id) { return images[id]; }

private:
	vk::UniqueSwapchainKHR swapchain;
	uint32_t image_count;
	std::vector<vk::Image> images;

	vk::Format format;
	vk::Extent2D extent;

	vk::UniqueRenderPass ui_render_pass;

	std::vector<VulkanFrame> frames;
	std::vector<FrameSemaphores> frame_semaphores;

	vk::UniqueDescriptorPool imgui_descriptor_pool;

	void create_swapchain(vk::Device device, vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, SDL_Window *window);
	void create_ui_render_pass(vk::Device device, vk::PhysicalDevice physical_device);
	void create_frames(vk::Device device, vk::PhysicalDevice physical_device, vk::CommandPool command_pool, vk::Sampler texture_sampler, vk::ImageView texture_image_view);
	void create_semaphores(vk::Device device);
	void create_imgui_context(SwapchainCreateInfo info);

	vk::SurfaceFormatKHR choose_surface_format(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);
	vk::PresentModeKHR choose_present_mode(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);
	vk::Extent2D choose_extent(const vk::SurfaceCapabilitiesKHR &capabilities, SDL_Window *window);
	vk::Format find_depth_format(vk::PhysicalDevice physical_device);
};

typedef std::unique_ptr<Swapchain> UniqueSwapchain;

#endif //VULKAN_SWAPCHAIN_H
