#include "VulkanApplication.h"

#pragma region HELPER

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
	std::cerr << pCallbackData->pMessage << std::endl
			  << std::endl;

	return VK_FALSE;
}

void populate_debug_messenger_createInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info) {
	create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debug_callback;
	create_info.pUserData = nullptr;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

static std::vector<char> read_file(const std::string &path) {
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

static void check_vk_result(VkResult err) {
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

#pragma endregion

#pragma region VULKAN FRAME

void VulkanFrame::init_frame(VkDevice device) {
	this->device = device;
}

void VulkanFrame::create_framebuffer(VkExtent2D swapchain_extent, VkRenderPass &render_pass, VkImageView &depth_image_view) {
	VkImageView attachments[] = {
		image_view,
		depth_image_view,
	};

	VkFramebufferCreateInfo framebuffer_ci{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = render_pass,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.width = swapchain_extent.width,
		.height = swapchain_extent.height,
		.layers = 1,
	};

	if (vkCreateFramebuffer(device, &framebuffer_ci, nullptr, &framebuffer) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create Framebuffer!");
	}
}

void VulkanFrame::create_command_buffers(VkCommandPool command_pool) {
	this->command_pool = command_pool;
	VkCommandBufferAllocateInfo ci{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	check_vk_result(vkAllocateCommandBuffers(device, &ci, &command_buffer));
}

void VulkanFrame::create_sync_objects() {
	VkFenceCreateInfo fence_ci{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	vkCreateFence(device, &fence_ci, nullptr, &fence);
}

void VulkanFrame::create_descriptor_set(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, VkSampler texture_sampler, VkImageView texture_image_view) {
	VkDescriptorSetAllocateInfo ai{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptor_set_layout,
	};
	check_vk_result(vkAllocateDescriptorSets(device, &ai, &descriptor_set));

	VkDescriptorBufferInfo bi{
		.buffer = uniform_buffer,
		.offset = 0,
		.range = sizeof(UniformBufferObject),
	};

	VkWriteDescriptorSet uniform_descriptor_write{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptor_set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &bi,
	};

	VkDescriptorImageInfo ii{
		.sampler = texture_sampler,
		.imageView = texture_image_view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};
	VkWriteDescriptorSet texture_descriptor_write{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptor_set,
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &ii,
	};

	std::array<VkWriteDescriptorSet, 2> descriptor_writes = { uniform_descriptor_write, texture_descriptor_write };
	vkUpdateDescriptorSets(device, uint32_t(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
}

void VulkanFrame::create_uniform_buffer(VkPhysicalDevice physical_device) {
	VkDeviceSize size = sizeof(UniformBufferObject);
	create_buffer(device, physical_device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffer, uniform_buffer_memory);
}

void VulkanFrame::delete_frame() {
	vkFreeMemory(device, uniform_buffer_memory, nullptr);
	vkDestroyBuffer(device, uniform_buffer, nullptr);

	vkDestroyFence(device, fence, nullptr);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
	vkDestroyFramebuffer(device, framebuffer, nullptr);
	vkDestroyImageView(device, image_view, nullptr);
}

#pragma endregion

#pragma region MAIN FLOW

void VulkanApplication::init_window() {
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
	window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
}

void VulkanApplication::init_imgui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO &io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForVulkan(window);
}

void VulkanApplication::cleanup_imgui() {
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void VulkanApplication::create_imgui_context() {
	{ // Descriptor pool
		std::array<VkDescriptorPoolSize, 11> pool_sizes{};
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		pool_sizes[0].descriptorCount = 1000;
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[1].descriptorCount = 1000;
		pool_sizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		pool_sizes[2].descriptorCount = 1000;
		pool_sizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		pool_sizes[3].descriptorCount = 1000;
		pool_sizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		pool_sizes[4].descriptorCount = 1000;
		pool_sizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		pool_sizes[5].descriptorCount = 1000;
		pool_sizes[6].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[6].descriptorCount = 1000;
		pool_sizes[7].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pool_sizes[7].descriptorCount = 1000;
		pool_sizes[8].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		pool_sizes[8].descriptorCount = 1000;
		pool_sizes[9].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		pool_sizes[9].descriptorCount = 1000;
		pool_sizes[10].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		pool_sizes[10].descriptorCount = 1000;

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * uint32_t(pool_sizes.size());
		pool_info.poolSizeCount = uint32_t(pool_sizes.size());
		pool_info.pPoolSizes = pool_sizes.data();
		check_vk_result(vkCreateDescriptorPool(device, &pool_info, nullptr, &imgui_descriptor_pool));
	}
	{ // Init vulkan
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = instance;
		init_info.PhysicalDevice = physical_device;
		init_info.Device = device;
		init_info.QueueFamily = queue_family_indices.graphics_family.value();
		init_info.Queue = graphics_queue;
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = imgui_descriptor_pool;
		init_info.Allocator = nullptr;
		init_info.MinImageCount = 2;
		init_info.ImageCount = uint32_t(frames.size());
		init_info.CheckVkResultFn = check_vk_result;

		ImGui_ImplVulkan_Init(&init_info, render_pass);
	}
	{ // Create font
		VkCommandBufferBeginInfo bi{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		vkBeginCommandBuffer(frames[0].command_buffer, &bi);
		ImGui_ImplVulkan_CreateFontsTexture(frames[0].command_buffer);
		vkEndCommandBuffer(frames[0].command_buffer);

		VkSubmitInfo submits{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &frames[0].command_buffer,
		};
		vkQueueSubmit(graphics_queue, 1, &submits, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphics_queue);
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
}

void VulkanApplication::cleanup_imgui_context() {
	ImGui_ImplVulkan_Shutdown();
	vkDestroyDescriptorPool(device, imgui_descriptor_pool, nullptr);
}

void VulkanApplication::init_vulkan() {
	create_instance();
	SDL_Log("VkInstance created!");
	setup_debug_messenger();
	create_surface();
	SDL_Log("Surface created!");
	pick_physical_device();
	SDL_Log("Physical device chosen!");
	create_logical_device();
	SDL_Log("Logical device created!");
	create_command_pools();
	SDL_Log("Command Pool created!");
	create_texture_image();
	SDL_Log("Image created!");
	create_texture_sampler();
	SDL_Log("Texture sampler created!");
	create_texture_image_view();
	SDL_Log("Image view created!");
	init_swapchain();

	SDL_ShowWindow(window);
}

void VulkanApplication::main_loop() {
	bool run = true;
	int last_time = SDL_GetTicks();
	float delta_time = 0;

	Input::start();
	start();

	while (run) {
		// Wait for V_Sync
		int time = SDL_GetTicks();
		delta_time = (time - last_time) / 1000.0f;
		if (wait_for_vsync) {
			if (delta_time <= 1.0f / float(FRAME_RATE))
				continue;
		}
		last_time = time;

		Input::update();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT) {
				run = false;
				SDL_Log("Exiting...");
			}
			if (event.type == SDL_WINDOWEVENT) {
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						framebuffer_rezised = true;
						break;
					case SDL_WINDOWEVENT_RESTORED:
						framebuffer_rezised = true;
						break;
				}
			}
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		update(delta_time);
		ImGui::Render();

		// Don't draw if the app is minimized
		if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)) {
			draw_frame();
		}

		vkDeviceWaitIdle(device);
	}
}

void VulkanApplication::draw_scene(VulkanFrame &frame) {
	void *data;
	vkMapMemory(device, frame.uniform_buffer_memory, 0, sizeof(transform), 0, &data);
	memcpy(data, &transform, sizeof(transform));
	vkUnmapMemory(device, frame.uniform_buffer_memory);

	vkCmdBindPipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(frame.command_buffer, 0, 1, &mesh_buffer, offsets);
	vkCmdBindIndexBuffer(frame.command_buffer, mesh_buffer, idx_offset, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &frame.descriptor_set, 0, nullptr);
	vkCmdDrawIndexed(frame.command_buffer, uint32_t(indices.size()), 1, 0, 0, 0);
}

void VulkanApplication::draw_ui(VulkanFrame &frame) {
	ImGui::Render();
	auto data = ImGui::GetDrawData();

	ImGui_ImplVulkan_RenderDrawData(data, frame.command_buffer);
}

void VulkanApplication::draw_frame() {
	// Get the image
	VkSemaphore image_acquired_semaphore = frame_semaphores[semaphore_index].image_aquired;
	VkSemaphore render_complete_semaphore = frame_semaphores[semaphore_index].render_complete;
	uint32_t image_id;
	auto result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &image_id); // Get next image and trigger image_available once ready
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		init_swapchain(true);
		return;
	}

	VulkanFrame *frame = &frames[image_id];

	// Wait for the frame to be finished
	vkWaitForFences(device, 1, &frame->fence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &frame->fence);

	{ // Begin cmd buffer
		VkCommandBufferBeginInfo buffer_ci{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
			.pInheritanceInfo = nullptr,
		};
		check_vk_result(vkBeginCommandBuffer(frame->command_buffer, &buffer_ci));
	}

	{ // Start Render Pass
		std::array<VkClearValue, 2> clear_values{};
		clear_values[0].color = { 0.f, 0.f, 0.f, 1.f };
		clear_values[1].depthStencil = { 1.f, 0 };
		VkRenderPassBeginInfo renderpass_ci{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = render_pass,
			.framebuffer = frame->framebuffer,
			.clearValueCount = uint32_t(clear_values.size()),
			.pClearValues = clear_values.data(),
		};
		renderpass_ci.renderArea.offset = { 0, 0 };
		renderpass_ci.renderArea.extent = swapchain_extent;

		vkCmdBeginRenderPass(frame->command_buffer, &renderpass_ci, VK_SUBPASS_CONTENTS_INLINE);
	}

	draw_scene(*frame); // Render 3D objects in the scene
	draw_ui(*frame); // Render UI

	vkCmdEndRenderPass(frame->command_buffer);
	check_vk_result(vkEndCommandBuffer(frame->command_buffer));

	{ // Submit Queue
		VkSemaphore semaphores[] = { image_acquired_semaphore };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore end_semaphores[] = { render_complete_semaphore };
		// Submit the rendering queue
		VkSubmitInfo si{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = semaphores, // wait for the image to be available
			.pWaitDstStageMask = wait_stages,
			.commandBufferCount = 1,
			.pCommandBuffers = &frame->command_buffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = end_semaphores, // Trigger the render finished semaphore once done
		};
		check_vk_result(vkQueueSubmit(graphics_queue, 1, &si, frame->fence));
	}

	{ // Present the image

		VkSemaphore end_semaphores[] = { render_complete_semaphore }; // Wait for the renderpass to be finished before rendering
		VkSwapchainKHR swapchains[] = { swapchain };
		VkPresentInfoKHR present_info{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = end_semaphores,
			.swapchainCount = 1,
			.pSwapchains = swapchains,
			.pImageIndices = &image_id,
		};
		result = vkQueuePresentKHR(present_queue, &present_info);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_rezised) {
			framebuffer_rezised = false;
			init_swapchain(true);
		}
	}

	semaphore_index = (semaphore_index + 1) % frames.size();
}

void VulkanApplication::cleanup() {
	SDL_Log("Cleaning up...");
	SDL_HideWindow(window);

	Input::cleanup();

	// VULKAN
	cleanup_swapchain();

	vkDestroySampler(device, texture_sampler, nullptr);

	vkDestroyImageView(device, texture_image_view, nullptr);
	vkDestroyImage(device, texture_image, nullptr);
	vkFreeMemory(device, texture_image_memory, nullptr);

	vkFreeMemory(device, mesh_buffer_memory, nullptr);
	vkDestroyBuffer(device, mesh_buffer, nullptr);

	vkDestroyCommandPool(device, graphics_command_pool, nullptr);
	vkDestroyCommandPool(device, transfer_command_pool, nullptr);

	vkDestroyDevice(device, nullptr);
	if (enable_validation_layers) {
		DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
	}
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	// IMGUI
	cleanup_imgui();

	// SDL
	SDL_DestroyWindow(window);
	SDL_Log("Cleanup done! Bye bye!");
	IMG_Quit();
	SDL_Quit();
}

void VulkanApplication::cleanup_swapchain() {
	for (auto &fs : frame_semaphores) {
		vkDestroySemaphore(device, fs.image_aquired, nullptr);
		vkDestroySemaphore(device, fs.render_complete, nullptr);
	}

	cleanup_imgui_context();

	for (auto frame : frames) {
		frame.delete_frame();
	}
	frames.clear();

	vkDestroyImageView(device, depth_image_view, nullptr);
	vkDestroyImage(device, depth_image, nullptr);
	vkFreeMemory(device, depth_image_memory, nullptr);

	vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
	vkDestroyDescriptorPool(device, descriptor_pool, nullptr);

	vkDestroyPipeline(device, graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

	vkDestroyRenderPass(device, render_pass, nullptr);
	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void VulkanApplication::init_swapchain(bool reset) {
	vkDeviceWaitIdle(device);

	if (reset)
		cleanup_swapchain();

	create_swapchain();
	SDL_Log("Swapchain created");
	uint32_t real_image_count = 0;
	vkGetSwapchainImagesKHR(device, swapchain, &real_image_count, nullptr);
	frames.resize(real_image_count);

	create_render_pass();
	SDL_Log("Render pass created");
	create_descriptors();
	SDL_Log("Descriptors created");
	create_graphics_pipeline();
	SDL_Log("Pipeline created");
	create_depth_resources();
	SDL_Log("Depth resources created");

	// Create frames
	std::vector<VkImage> swapchain_images;
	swapchain_images.resize(real_image_count);
	vkGetSwapchainImagesKHR(device, swapchain, &real_image_count, swapchain_images.data());

	for (uint32_t i = 0; i < real_image_count; i++) {
		frames[i].init_frame(device);
		create_image_view(device, swapchain_images[i], swapchain_format, VK_IMAGE_ASPECT_COLOR_BIT, &frames[i].image_view);
		frames[i].create_framebuffer(swapchain_extent, render_pass, depth_image_view);
		frames[i].create_command_buffers(graphics_command_pool);
		frames[i].create_sync_objects();
		frames[i].create_uniform_buffer(physical_device);
		frames[i].create_descriptor_set(descriptor_pool, descriptor_set_layout, texture_sampler, texture_image_view);
	}

	create_imgui_context();
	//SDL_Log("imgui created");

	create_sync_objects();
	SDL_Log("Sync objects created!");
}

#pragma endregion

#pragma region VULKAN

void VulkanApplication::create_instance() {
	if (enable_validation_layers && !check_validation_layer_support())
		throw std::runtime_error("validation layers requested, but none are available");

	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Vulkan";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "sl3dge";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	// Get extension count & names
	auto required_extensions = get_required_extensions();
	create_info.enabledExtensionCount = uint32_t(required_extensions.size());
	create_info.ppEnabledExtensionNames = required_extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
	if (enable_validation_layers) {
		populate_debug_messenger_createInfo(debug_create_info);
		create_info.enabledLayerCount = uint32_t(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
		create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
	} else {
		create_info.enabledLayerCount = 0;
		create_info.pNext = nullptr;
	}

	if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vkinstance");
	}

	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> extensions(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
}

std::vector<const char *> VulkanApplication::get_required_extensions() {
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

void VulkanApplication::create_surface() {
	if (SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE) {
		throw std::runtime_error("unable to create window surface");
	}
}

#pragma endregion

#pragma region DEBUG

bool VulkanApplication::check_validation_layer_support() {
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

void VulkanApplication::setup_debug_messenger() {
	if (!enable_validation_layers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT create_info;
	populate_debug_messenger_createInfo(create_info);

	if (CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
		throw std::runtime_error("unable to create debug messenger");
	}
}

#pragma endregion

#pragma region DEVICES

void VulkanApplication::pick_physical_device() {
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if (device_count == 0) {
		throw std::runtime_error("no GPUs with vulkan support");
	}
	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	for (const auto &device : devices) {
		if (is_device_suitable(device)) {
			physical_device = device;
			break;
		}
	}

	if (physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("No GPUs with vulkan support");
	}
}

bool VulkanApplication::is_device_suitable(VkPhysicalDevice device) {
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceFeatures(device, &device_features);

	QueueFamilyIndices indices = find_queue_families(device);

	bool extensions_supported = check_device_extension_support(device);

	bool swapchain_adequate = false;
	if (extensions_supported) {
		SwapChainSupportDetails swapchain_support = query_swap_chain_support(device);
		swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
	}

	return indices.is_complete() && extensions_supported && swapchain_adequate && device_features.samplerAnisotropy;
}

bool VulkanApplication::check_device_extension_support(VkPhysicalDevice device) {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	for (const auto &extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}

QueueFamilyIndices VulkanApplication::find_queue_families(VkPhysicalDevice device) {
	QueueFamilyIndices indices;
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	auto queue_family_properties = std::vector<VkQueueFamilyProperties>(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties.data());

	int i = 0;
	for (const auto &queue_family : queue_family_properties) {
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = i;
		}

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

		if (present_support) {
			indices.present_family = i;
		}

		if ((queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			indices.transfer_family = i;
		}

		if (indices.is_complete())
			break;

		i++;
	}
	return indices;
}

void VulkanApplication::create_logical_device() {
	queue_family_indices = find_queue_families(physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = { queue_family_indices.graphics_family.value(), queue_family_indices.present_family.value(), queue_family_indices.transfer_family.value() };

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features{
		.samplerAnisotropy = VK_TRUE,
	};

	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();

	if (enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	} else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device.");
	}

	vkGetDeviceQueue(device, queue_family_indices.graphics_family.value(), 0, &graphics_queue);
	vkGetDeviceQueue(device, queue_family_indices.present_family.value(), 0, &present_queue);
	vkGetDeviceQueue(device, queue_family_indices.transfer_family.value(), 0, &transfer_queue);
}

#pragma endregion

#pragma region SWAPCHAIN

VkSurfaceFormatKHR VulkanApplication::choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats) {
	for (const auto &format : available_formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			//SDL_Log("Format: VK_FORMAT_B8G8R8A8_SRGB");
			return format;
		}
	}
	return available_formats[0];
}

VkPresentModeKHR VulkanApplication::choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> &available_present_modes) {
	for (const auto &mode : available_present_modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			//SDL_Log("Present mode: VK_PRESENT_MODE_MAILBOX_KHR");
			return mode;
		}
	}
	SDL_Log("Present mode: VK_PRESENT_MODE_FIFO_KHR");
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanApplication::choose_swapchain_extent(const VkSurfaceCapabilitiesKHR &capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		//SDL_Log("Resolution: %ix%i", capabilities.currentExtent.width, capabilities.currentExtent.height);
		return capabilities.currentExtent;
	} else {
		int w, h;
		SDL_Vulkan_GetDrawableSize(window, &w, &h);
		VkExtent2D extent = { uint32_t(w), uint32_t(h) };
		extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
		extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));
		//SDL_Log("Resolution: %ix%i", extent.width, extent.height);
		return extent;
	}
}

SwapChainSupportDetails VulkanApplication::query_swap_chain_support(VkPhysicalDevice device) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

	if (present_mode_count != 0) {
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
	}

	return details;
}

void VulkanApplication::create_swapchain() {
	SwapChainSupportDetails support = query_swap_chain_support(physical_device);

	auto surface_fmt = choose_swapchain_surface_format(support.formats);
	auto present_mode = choose_swapchain_present_mode(support.present_modes);
	auto extent = choose_swapchain_extent(support.capabilities);

	uint32_t image_count = support.capabilities.minImageCount + 1;
	if (support.capabilities.maxImageCount > 0 && image_count > support.capabilities.maxImageCount) {
		image_count = support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_fmt.format;
	create_info.imageColorSpace = surface_fmt.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto indices = find_queue_families(physical_device);
	uint32_t queue_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

	if (indices.present_family != indices.graphics_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_indices;
	} else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;
	if (vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain)) {
		throw std::runtime_error("Unable to create swapchain!");
	}

	swapchain_format = surface_fmt.format;
	swapchain_extent = extent;
}

#pragma endregion

#pragma region RENDERING

VkShaderModule VulkanApplication::create_shader_module(const std::vector<char> &code) {
	VkShaderModuleCreateInfo ci{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t *>(code.data())
	};

	VkShaderModule shader_module;

	if (vkCreateShaderModule(device, &ci, nullptr, &shader_module) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create shader module!");
	};
	return shader_module;
}

void VulkanApplication::create_render_pass() {
	VkAttachmentDescription color_attachment{
		.format = swapchain_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentReference color_attachment_ref{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentDescription depth_attachment{
		.format = find_depth_format(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depth_attachment_ref{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
		.pDepthStencilAttachment = &depth_attachment_ref,
	};

	VkSubpassDependency dependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	};

	std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };
	VkRenderPassCreateInfo renderpass_ci{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = uint32_t(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
	};

	if (vkCreateRenderPass(device, &renderpass_ci, nullptr, &render_pass) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create Render Pass!");
	}
}

void VulkanApplication::create_graphics_pipeline() {
	auto vertex_shader_code = read_file("resources/shaders/triangle.vert.spv");
	auto fragment_shader_code = read_file("resources/shaders/triangle.frag.spv");

	VkShaderModule vertex_shader_module = create_shader_module(vertex_shader_code);
	VkShaderModule fragment_shader_module = create_shader_module(fragment_shader_code);

	VkPipelineShaderStageCreateInfo vertex_shader_stage_ci{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertex_shader_module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo fragment_shader_stage_ci{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragment_shader_module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo stages_ci[] = { vertex_shader_stage_ci, fragment_shader_stage_ci };

	uint32_t attribute_count = uint32_t(Vertex::get_attribute_descriptions().size());
	VkVertexInputBindingDescription descriptions[] = { Vertex::get_binding_description() };

	VkPipelineVertexInputStateCreateInfo vertex_input_ci{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = descriptions,
		.vertexAttributeDescriptionCount = attribute_count,
		.pVertexAttributeDescriptions = Vertex::get_attribute_descriptions().data(),
	};

	VkPipelineInputAssemblyStateCreateInfo input_ci{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkViewport viewport{
		.x = 0.f,
		.y = 0.f,
		.width = (float)swapchain_extent.width,
		.height = (float)swapchain_extent.height,
		.minDepth = 0.f,
		.maxDepth = 1.f
	};

	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = swapchain_extent
	};

	VkPipelineViewportStateCreateInfo viewport_state_ci{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizer{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.f,
		.depthBiasClamp = 0.f,
		.depthBiasSlopeFactor = 0.f,
		.lineWidth = 1.f,
	};

	VkPipelineMultisampleStateCreateInfo multisampling{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {}, // Optional
		.back = {}, // Optional
		.minDepthBounds = 0.0f, // Optional
		.maxDepthBounds = 1.0f, // Optional
	};

	VkPipelineColorBlendAttachmentState color_blend_attachement{
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo color_blending{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachement,
	};

	VkPipelineLayoutCreateInfo pipeline_layout_ci{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptor_set_layout,
	};

	if (vkCreatePipelineLayout(device, &pipeline_layout_ci, nullptr, &pipeline_layout) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipeline_ci{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = stages_ci,
		.pVertexInputState = &vertex_input_ci,
		.pInputAssemblyState = &input_ci,
		.pTessellationState = nullptr,
		.pViewportState = &viewport_state_ci,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = &depth_stencil,
		.pColorBlendState = &color_blending,
		.pDynamicState = nullptr,
		.layout = pipeline_layout,
		.renderPass = render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &graphics_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create Graphics Pipeline!");
	}

	vkDestroyShaderModule(device, vertex_shader_module, nullptr);
	vkDestroyShaderModule(device, fragment_shader_module, nullptr);
}

#pragma endregion

#pragma region COMMAND BUFFERS

void VulkanApplication::create_command_pool(Uint32 queue_family_index, VkCommandPoolCreateFlags flags, VkCommandPool *command_pool) {
	VkCommandPoolCreateInfo ci{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = flags,
		.queueFamilyIndex = queue_family_index,
	};

	check_vk_result(vkCreateCommandPool(device, &ci, nullptr, command_pool));
}

void VulkanApplication::create_command_pools() {
	create_command_pool(queue_family_indices.graphics_family.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, &graphics_command_pool);
	create_command_pool(queue_family_indices.transfer_family.value(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, &transfer_command_pool);
}

void VulkanApplication::create_command_buffer(VkCommandPool pool, VkCommandBuffer *c_buffer) {
	VkCommandBufferAllocateInfo ai{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	check_vk_result(vkAllocateCommandBuffers(device, &ai, c_buffer));
}

VkCommandBuffer VulkanApplication::begin_graphics_command_buffer() {
	VkCommandBufferAllocateInfo ai{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = graphics_command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer c_buffer;
	vkAllocateCommandBuffers(device, &ai, &c_buffer);

	VkCommandBufferBeginInfo bi{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(c_buffer, &bi);

	return c_buffer;
}

void VulkanApplication::end_graphics_command_buffer(VkCommandBuffer c_buffer) {
	vkEndCommandBuffer(c_buffer);

	VkSubmitInfo submits{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &c_buffer,
	};
	vkQueueSubmit(graphics_queue, 1, &submits, VK_NULL_HANDLE);
}

VkCommandBuffer VulkanApplication::begin_transfer_command_buffer() {
	VkCommandBufferAllocateInfo ai{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = transfer_command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer c_buffer;
	vkAllocateCommandBuffers(device, &ai, &c_buffer);

	VkCommandBufferBeginInfo bi{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(c_buffer, &bi);

	return c_buffer;
}

void VulkanApplication::end_transfer_command_buffer(VkCommandBuffer c_buffer) {
	vkEndCommandBuffer(c_buffer);

	VkSubmitInfo submits{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &c_buffer,
	};
	vkQueueSubmit(transfer_queue, 1, &submits, VK_NULL_HANDLE);
}

#pragma endregion

#pragma region MESH BUFFER

void VulkanApplication::create_sync_objects() {
	frame_semaphores.resize(frames.size());

	for (auto &fs : frame_semaphores) {
		VkSemaphoreCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		vkCreateSemaphore(device, &ci, nullptr, &fs.image_aquired);
		vkCreateSemaphore(device, &ci, nullptr, &fs.render_complete);
	}
}

// Copy a buffer from a staging buffer in the transfer queue to a buffer in the graphics queue transferring ownership.
void VulkanApplication::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
	auto send_cbuffer = begin_transfer_command_buffer();

	VkBufferCopy regions{
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size,
	};
	vkCmdCopyBuffer(send_cbuffer, src, dst, 1, &regions);

	VkBufferMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask = 0,
		.srcQueueFamilyIndex = queue_family_indices.transfer_family.value(),
		.dstQueueFamilyIndex = queue_family_indices.graphics_family.value(),
		.buffer = dst,
		.offset = 0,
		.size = size,
	};
	vkCmdPipelineBarrier(send_cbuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 1, &barrier, 0, VK_NULL_HANDLE);
	end_transfer_command_buffer(send_cbuffer);

	auto receive_cbuffer = begin_graphics_command_buffer();

	// reuse the old barrier create info
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	vkCmdPipelineBarrier(receive_cbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, VK_NULL_HANDLE, 1, &barrier, 0, VK_NULL_HANDLE);
	end_graphics_command_buffer(receive_cbuffer);

	vkQueueWaitIdle(transfer_queue);
	vkFreeCommandBuffers(device, transfer_command_pool, 1, &send_cbuffer);

	SDL_Log("Buffer copied!");
}

void VulkanApplication::create_mesh_buffer() {
	VkDeviceSize vertex_size = idx_offset = sizeof(vertices[0]) * uint32_t(vertices.size());
	VkDeviceSize index_size = sizeof(indices[0]) * indices.size();

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(device, physical_device, vertex_size + index_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	void *data;
	// Copy vtx data
	vkMapMemory(device, staging_buffer_memory, 0, vertex_size, 0, &data);
	memcpy(data, vertices.data(), size_t(vertex_size));
	vkUnmapMemory(device, staging_buffer_memory);
	// Copy idx data
	vkMapMemory(device, staging_buffer_memory, idx_offset, index_size, 0, &data);
	memcpy(data, indices.data(), size_t(index_size));
	vkUnmapMemory(device, staging_buffer_memory);

	create_buffer(device, physical_device, vertex_size + index_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh_buffer, mesh_buffer_memory);
	copy_buffer(staging_buffer, mesh_buffer, vertex_size + index_size);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_buffer_memory, nullptr);
}

#pragma endregion

#pragma region DESCRIPTORS

void VulkanApplication::create_descriptors() {
	{
		VkDescriptorSetLayoutBinding ubo_layout_binding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		};

		VkDescriptorSetLayoutBinding sampler_layout_binding{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		};

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };
		VkDescriptorSetLayoutCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = uint32_t(bindings.size()),
			.pBindings = bindings.data(),
		};
		vkCreateDescriptorSetLayout(device, &ci, nullptr, &descriptor_set_layout);
	}
	{
		std::array<VkDescriptorPoolSize, 2> pool_sizes{};
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = uint32_t(frames.size());
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[1].descriptorCount = uint32_t(frames.size());

		VkDescriptorPoolCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = uint32_t(frames.size()),
			.poolSizeCount = uint32_t(pool_sizes.size()),
			.pPoolSizes = pool_sizes.data(),
		};

		check_vk_result(vkCreateDescriptorPool(device, &ci, nullptr, &descriptor_pool));
	}
}

#pragma endregion

#pragma region TEXTURE

void VulkanApplication::create_texture_image() {
	// Load image
	SDL_Surface *surf = IMG_Load("resources/textures/viking_room.png");
	surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ABGR8888, 0);

	if (surf == NULL) {
		SDL_LogError(0, "%s", SDL_GetError());
		throw std::runtime_error("Unable to create texture");
	}

	VkFormat format = get_vk_format(surf->format);
	create_image(surf->w, surf->h, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image, texture_image_memory);

	// Create data transfer buffer
	VkDeviceSize img_size = surf->w * surf->h * surf->format->BytesPerPixel;
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(device, physical_device, img_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	// Copy image data to buffer
	void *data;
	vkMapMemory(this->device, staging_buffer_memory, 0, img_size, 0, &data);
	memcpy(data, surf->pixels, img_size);
	vkUnmapMemory(this->device, staging_buffer_memory);

	// Copy the buffer to the image and send it to graphics queue
	auto transfer_cbuffer = begin_transfer_command_buffer();
	transition_image_layout(transfer_cbuffer, texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copy_buffer_to_image(transfer_cbuffer, staging_buffer, texture_image, surf->w, surf->h);
	transition_image_layout(transfer_cbuffer, texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	end_transfer_command_buffer(transfer_cbuffer);

	// Receive the image on the graphics queue
	auto receive_cbuffer = begin_graphics_command_buffer();
	VkImageMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = queue_family_indices.transfer_family.value(),
		.dstQueueFamilyIndex = queue_family_indices.graphics_family.value(),
		.image = texture_image,
		.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 },
	};
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	vkCmdPipelineBarrier(receive_cbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &barrier);
	end_graphics_command_buffer(receive_cbuffer);

	// Wait for everything to process
	vkQueueWaitIdle(transfer_queue);
	vkFreeCommandBuffers(device, transfer_command_pool, 1, &transfer_cbuffer);
	vkQueueWaitIdle(graphics_queue);
	vkFreeCommandBuffers(device, graphics_command_pool, 1, &receive_cbuffer);

	SDL_FreeSurface(surf);
	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_buffer_memory, nullptr);
}

void VulkanApplication::create_image(uint32_t w, uint32_t h, VkFormat fmt, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory) {
	// Create VkImage

	VkImageCreateInfo ci{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = fmt,
		.extent = { w, h, 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};
	if (vkCreateImage(this->device, &ci, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create texture");
	}

	// Allocate VkImage memory
	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(device, image, &mem_reqs);
	VkMemoryAllocateInfo ai{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = find_memory_type(physical_device, mem_reqs.memoryTypeBits, properties),
	};

	if (vkAllocateMemory(this->device, &ai, nullptr, &image_memory) != VK_SUCCESS) {
		throw std::runtime_error("Unable to allocate memory for texture");
	}

	vkBindImageMemory(this->device, image, image_memory, 0);
}

void VulkanApplication::transition_image_layout(VkCommandBuffer c_buffer, VkImage image, VkFormat format, VkImageLayout from, VkImageLayout to) {
	VkImageMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = from,
		.newLayout = to,
		.srcQueueFamilyIndex = queue_family_indices.transfer_family.value(),
		.dstQueueFamilyIndex = queue_family_indices.transfer_family.value(),
		.image = image,
		.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 },
	};

	VkPipelineStageFlags src_stage;
	VkPipelineStageFlags dst_stage;
	if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		barrier.srcQueueFamilyIndex = queue_family_indices.transfer_family.value();
		barrier.dstQueueFamilyIndex = queue_family_indices.transfer_family.value();

	} else if (from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = 0;

		barrier.srcQueueFamilyIndex = queue_family_indices.transfer_family.value();
		barrier.dstQueueFamilyIndex = queue_family_indices.graphics_family.value();

		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	} else {
		throw std::runtime_error("Unsupported layout transition");
	}

	vkCmdPipelineBarrier(c_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanApplication::copy_buffer_to_image(VkCommandBuffer c_buffer, VkBuffer buffer, VkImage image, uint32_t w, uint32_t h) {
	VkBufferImageCopy region{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,

		.imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
		.imageOffset = { 0, 0, 0 },
		.imageExtent = { w, h, 1 },
	};
	vkCmdCopyBufferToImage(c_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void VulkanApplication::create_texture_image_view() {
	create_image_view(device, texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, &texture_image_view);
}

void VulkanApplication::create_texture_sampler() {
	VkSamplerCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	ci.magFilter = VK_FILTER_LINEAR;
	ci.minFilter = VK_FILTER_LINEAR;
	ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	ci.anisotropyEnable = VK_TRUE;
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physical_device, &properties);
	ci.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	ci.unnormalizedCoordinates = VK_FALSE;
	ci.compareEnable = VK_FALSE;
	ci.compareOp = VK_COMPARE_OP_ALWAYS;
	ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	ci.mipLodBias = 0.0f;
	ci.minLod = 0.0f;
	ci.maxLod = 0.0f;

	if (vkCreateSampler(device, &ci, nullptr, &texture_sampler) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create texture sampler!");
	}
}
#pragma endregion

#pragma region DEPTH BUFFER

void VulkanApplication::create_depth_resources() {
	VkFormat depth_format = find_depth_format();

	create_image(swapchain_extent.width, swapchain_extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image, depth_image_memory);
	create_image_view(device, depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, &depth_image_view);
}

VkFormat VulkanApplication::find_supported_format(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat VulkanApplication::find_depth_format() {
	return find_supported_format(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool VulkanApplication::has_stencil_component(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

#pragma endregion
