#include "VulkanApplication.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

//
// Init & Cleanup
//
void VulkanApplication::init_window() {
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
	window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
}

void VulkanApplication::init_vulkan() {
	// Init dispatcher
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	// Instance
	create_instance();
	SDL_Log("VkInstance created!");

	// Logger
	if (enable_validation_layers) {
		SDL_Log("DEBUG");
		vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
															 vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
															 vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
		vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
															 vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
															 vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

		debug_messenger = instance->createDebugUtilsMessengerEXTUnique(vk::DebugUtilsMessengerCreateInfoEXT({}, severity_flags, message_type_flags, &debug_callback));
	}

	// Surface
	{
		VkSurfaceKHR surf;
		if (SDL_Vulkan_CreateSurface(window, *instance, &surf) != SDL_TRUE) {
			throw std::runtime_error("Unable to create window surface");
		}
		vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderDynamic> surface_deleter(*instance);
		surface = vk::UniqueSurfaceKHR(surf, surface_deleter);
		SDL_Log("Surface created!");
	}

	pick_physical_device();
	SDL_Log("Using %s", physical_device.getProperties().deviceName);
	create_logical_device();
	SDL_Log("Logical device created!");

	// Command Pools
	graphics_command_pool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_family_indices.graphics_family.value()));
	transfer_command_pool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient, queue_family_indices.transfer_family.value()));
	SDL_Log("Command Pools created!");

	// Texture
	create_texture_image();
	SDL_Log("Image created!");
	create_texture_sampler();
	SDL_Log("Texture sampler created!");
	texture_image_view = create_image_view(*device, texture_image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	SDL_Log("Image view created!");

	init_swapchain();

	SDL_ShowWindow(window);
}

void VulkanApplication::cleanup() {
	SDL_Log("Cleaning up...");
	SDL_HideWindow(window);

	Input::cleanup();

	// VULKAN
	cleanup_swapchain();


	vkDestroyImage(*device, texture_image, nullptr);
	vkFreeMemory(*device, texture_image_memory, nullptr);

	vkFreeMemory(*device, mesh_buffer_memory, nullptr);
	vkDestroyBuffer(*device, mesh_buffer, nullptr);

	/*
	vkDestroyCommandPool(*device, graphics_command_pool, nullptr);
	vkDestroyCommandPool(*device, transfer_command_pool, nullptr);
	*/
	//vkDestroySurfaceKHR(*instance, surface, nullptr);

	// IMGUI
	cleanup_imgui();

	// SDL
	SDL_DestroyWindow(window);
	SDL_Log("Cleanup done! Bye bye!");
	IMG_Quit();
	SDL_Quit();
}

//
// Loop
//
void VulkanApplication::main_loop() {
	bool run = true;
	int last_time = SDL_GetTicks();
	float delta_time;

	Input::start();
	start();

	while (run) {
		// Wait for V_Sync
		int time = SDL_GetTicks();
		delta_time = (time - last_time) / 1000.0f;
		if (wait_for_vsync) {
			if (delta_time <= 1.0f / float(FRAME_RATE)) {
				continue;
			}
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
		device->waitIdle();
	}
}

void VulkanApplication::draw_scene(VulkanFrame &frame) {
	void *data = device->mapMemory(frame.uniform_buffer_memory, 0, sizeof(vubo), {});
	memcpy(data, &vubo, sizeof(vubo));
	device->unmapMemory(frame.uniform_buffer_memory);

	data = device->mapMemory(frame.uniform_buffer_memory, sizeof(vubo), sizeof(fubo), {});
	memcpy(data, &fubo, sizeof(fubo));
	device->unmapMemory(frame.uniform_buffer_memory);

	frame.command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline);
	frame.command_buffer->bindVertexBuffers(0, { mesh_buffer }, { 0 });
	frame.command_buffer->bindIndexBuffer(mesh_buffer, idx_offset, vk::IndexType::eUint32);
	frame.command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, frame.descriptor_set.get(), nullptr);
	frame.command_buffer->drawIndexed(indices.size(), 1, 0, 0, 0);
}

void VulkanApplication::draw_frame() {
	// Get the image
	vk::Semaphore image_acquired_semaphore = *frame_semaphores[semaphore_index].image_aquired;
	vk::Semaphore render_complete_semaphore = *frame_semaphores[semaphore_index].render_complete;

	// Get next image and trigger image_available once ready
	auto [result, image_id] = device->acquireNextImageKHR(*swapchain, UINT64_MAX, image_acquired_semaphore, nullptr);

	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
		init_swapchain(true);
		return;
	}

	VulkanFrame *frame = &frames[image_id];

	// Wait for the frame to be finished
	if (device->waitForFences(frame->fence.get(), true, UINT64_MAX) != vk::Result::eSuccess) {
		throw std::runtime_error("Waiting for frame timed out!");
	}
	device->resetFences(frame->fence.get());
	frame->command_buffer->begin(vk::CommandBufferBeginInfo({}, nullptr));

	{ // Start Render Pass
		std::array<vk::ClearValue, 2> clear_values{};
		clear_values[0].color = vk::ClearColorValue(std::array<float, 4>{ 0.f, 0.f, 0.f, 1.f });
		clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.f, 0);
		vk::Rect2D rect(vk::Offset2D(0, 0), swapchain_extent);

		frame->command_buffer->beginRenderPass(vk::RenderPassBeginInfo(*render_pass, frame->framebuffer.get(), rect, clear_values), vk::SubpassContents::eInline);
	}

	draw_scene(*frame); // Render 3D objects in the scene
	draw_ui(*frame); // Render UI
	frame->command_buffer->endRenderPass();
	frame->command_buffer->end();

	// Submit Queue
	{
		vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo si(image_acquired_semaphore, flags, frame->command_buffer.get(), render_complete_semaphore);
		graphics_queue.submit(si, frame->fence.get());
	}
	// Present
	{
		result = present_queue.presentKHR(vk::PresentInfoKHR(render_complete_semaphore, *swapchain, image_id, {}));

		if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebuffer_rezised) {
			framebuffer_rezised = false;
			init_swapchain(true);
		}
	}
	semaphore_index = (semaphore_index + 1) % frames.size();
}

//
//	Device
//

void VulkanApplication::create_instance() {
	if (enable_validation_layers && !check_validation_layer_support()) {
		throw std::runtime_error("Validation layers requested, but none are available");
	}

	vk::ApplicationInfo app_info("Vulkan", 1, "sl3dge", 1, VK_API_VERSION_1_2);

	auto req_extensions = get_required_extensions(window);
	vk::InstanceCreateInfo ci({}, &app_info, req_validation_layers, req_extensions);

	instance = vk::createInstanceUnique(ci);

	VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

	auto extentions = vk::enumerateInstanceExtensionProperties();
}

void VulkanApplication::pick_physical_device() {
	auto devices = instance->enumeratePhysicalDevices();
	SDL_Log("%zu devices found", devices.size());
	for (const auto &p_device : devices) {
		if (!p_device.getFeatures2().features.samplerAnisotropy) {
			continue;
		}

		// Queues
		QueueFamilyIndices family_indices = find_queue_families(p_device, *surface);
		if (!family_indices.is_complete()) {
			continue;
		}

		// Extensions
		auto available_extensions = p_device.enumerateDeviceExtensionProperties();
		std::set<std::string> required_extensions(req_device_extensions.begin(), req_device_extensions.end());
		for (const auto &extension : available_extensions) {
			required_extensions.erase(extension.extensionName);
		}
		if (!required_extensions.empty()) {
			continue;
		}

		// Swapchain
		bool swapchain_adequate = !p_device.getSurfaceFormatsKHR(*surface).empty() && !p_device.getSurfacePresentModesKHR(*surface).empty();
		if (!swapchain_adequate) {
			continue;
		}

		// This p_device is ok
		physical_device = p_device;
		break;
	}

	if (!physical_device) {
		throw std::runtime_error("No GPUs with requested support");
	}
}

void VulkanApplication::create_logical_device() {
	queue_family_indices = find_queue_families(physical_device, *surface);

	std::vector<vk::DeviceQueueCreateInfo> req_queues;
	std::set<uint32_t> unique_queue_families = { queue_family_indices.graphics_family.value(), queue_family_indices.present_family.value(), queue_family_indices.transfer_family.value() };
	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info{};
		// TODO : VkHpp that
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		req_queues.push_back(queue_create_info);
	}

	vk::PhysicalDeviceFeatures req_device_features;
	req_device_features.samplerAnisotropy = true;

	device = physical_device.createDeviceUnique(vk::DeviceCreateInfo({}, req_queues, req_validation_layers, req_device_extensions, &req_device_features));

	VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

	graphics_queue = device->getQueue(queue_family_indices.graphics_family.value(), 0);
	present_queue = device->getQueue(queue_family_indices.present_family.value(), 0);
	transfer_queue = device->getQueue(queue_family_indices.transfer_family.value(), 0);
}

//
//	Swapchain
//

void VulkanApplication::init_swapchain(bool reset) {
	device->waitIdle();

	if (reset) {
		cleanup_swapchain();
	}

	create_swapchain();
	SDL_Log("Swapchain created");
	uint32_t real_image_count = device->getSwapchainImagesKHR(*swapchain).size();
	frames.resize(real_image_count);

	create_render_pass();
	SDL_Log("Render pass created");
	create_descriptors();
	SDL_Log("Descriptors created");
	create_depth_resources();
	SDL_Log("Depth resources created");
	create_graphics_pipeline();
	SDL_Log("Pipeline created");

	// Create frames
	std::vector<VkImage> swapchain_images;
	swapchain_images.resize(real_image_count);
	vkGetSwapchainImagesKHR(*device, *swapchain, &real_image_count, swapchain_images.data());
	for (uint32_t i = 0; i < real_image_count; i++) {
		frames[i].init_frame(*device);
		frames[i].image_view = create_image_view(*device, swapchain_images[i], swapchain_format, vk::ImageAspectFlagBits::eColor);
		frames[i].create_framebuffer(swapchain_extent, *render_pass, *depth_image_view);
		frames[i].create_command_buffers(*graphics_command_pool);
		frames[i].create_sync_objects();
		frames[i].create_uniform_buffer(physical_device);
		frames[i].create_descriptor_set(*descriptor_pool, *descriptor_set_layout, *texture_sampler, *texture_image_view);
	}

	create_imgui_context();

	create_sync_objects();
	SDL_Log("Sync objects created!");
}

void VulkanApplication::create_swapchain() {
	auto capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);

	auto surface_fmt = choose_swapchain_surface_format(physical_device, *surface);
	auto present_mode = choose_swapchain_present_mode(physical_device, *surface);
	auto extent = choose_swapchain_extent(capabilities, window);

	uint32_t image_count = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
		image_count = capabilities.maxImageCount;
	}
	auto family_indices = find_queue_families(physical_device, *surface);
	std::array<const uint32_t, 2> queue_indices({ family_indices.graphics_family.value(), family_indices.present_family.value() });

	vk::SwapchainCreateInfoKHR ci({},
			*surface,
			image_count,
			surface_fmt.format,
			surface_fmt.colorSpace,
			extent,
			1,
			{ vk::ImageUsageFlagBits::eColorAttachment },
			vk::SharingMode::eExclusive,
			queue_indices,
			capabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			present_mode,
			VK_TRUE);
	swapchain = device->createSwapchainKHRUnique(ci);
	swapchain_format = surface_fmt.format;
	swapchain_extent = extent;
}

void VulkanApplication::cleanup_swapchain() {
	frame_semaphores.clear();

	cleanup_imgui_context();

	for (auto &frame : frames) {
		frame.delete_frame();
	}
	frames.clear();

	vkDestroyImage(*device, depth_image, nullptr);
	vkFreeMemory(*device, depth_image_memory, nullptr);
}

//
//	Rendering
//

void VulkanApplication::create_render_pass() {
	std::vector<vk::AttachmentDescription> attachments;
	vk::AttachmentDescription color_attachment(
			{},
			swapchain_format,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR);
	vk::AttachmentReference color_ref(0U, vk::ImageLayout::eColorAttachmentOptimal);
	attachments.push_back(color_attachment);

	vk::AttachmentDescription depth_attachment(
			{},
			find_depth_format(),
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::AttachmentReference depth_ref(1U, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	attachments.push_back(depth_attachment);

	vk::SubpassDescription subpass(
			{},
			vk::PipelineBindPoint::eGraphics,
			nullptr, color_ref,
			nullptr, &depth_ref,
			nullptr);

	vk::SubpassDependency dependency(
			VK_SUBPASS_EXTERNAL,
			0U,
			{ vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests },
			{ vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests },
			{},
			{ vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite },
			{});

	render_pass = device->createRenderPassUnique(vk::RenderPassCreateInfo({}, attachments, subpass, dependency));
}

void VulkanApplication::create_graphics_pipeline() {
	auto vertex_shader_code = read_file("resources/shaders/triangle.vert.spv");
	auto fragment_shader_code = read_file("resources/shaders/triangle.frag.spv");

	auto vtx_shader = device->createShaderModule(vk::ShaderModuleCreateInfo({}, vertex_shader_code.size(), reinterpret_cast<uint32_t *>(vertex_shader_code.data())));
	auto frag_shader = device->createShaderModule(vk::ShaderModuleCreateInfo({}, fragment_shader_code.size(), reinterpret_cast<uint32_t *>(fragment_shader_code.data())));

	auto stages = {
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vtx_shader, "main", nullptr),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, frag_shader, "main", nullptr)
	};
	auto a = Vertex::get_binding_description();
	auto b = Vertex::get_attribute_descriptions();
	vk::PipelineVertexInputStateCreateInfo vertex_input({}, a, b);

	vk::PipelineInputAssemblyStateCreateInfo input_ci({}, vk::PrimitiveTopology::eTriangleList, false);

	vk::Viewport viewport(0, 0, swapchain_extent.width, swapchain_extent.height, 0, 1);
	vk::Rect2D scissor({ 0, 0 }, swapchain_extent);
	vk::PipelineViewportStateCreateInfo viewport_state_ci({}, viewport, scissor);

	vk::PipelineRasterizationStateCreateInfo rasterizer({}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, false, 0, 0, 0, 1);
	vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, false, 1, nullptr, false, false);
	vk::PipelineDepthStencilStateCreateInfo depth_stencil({}, true, true, vk::CompareOp::eLess, false, false, {}, {}, 0, 1);

	vk::PipelineColorBlendAttachmentState color_blend_attachment(
			false,
			vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
			vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
			{ vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA });

	vk::PipelineColorBlendStateCreateInfo color_blending({}, false, vk::LogicOp::eCopy, color_blend_attachment, {});

	pipeline_layout = device->createPipelineLayout(vk::PipelineLayoutCreateInfo({}, *descriptor_set_layout));

	vk::GraphicsPipelineCreateInfo pipeline_ci(
			{},
			stages,
			&vertex_input,
			&input_ci,
			nullptr,
			&viewport_state_ci,
			&rasterizer,
			&multisampling,
			&depth_stencil,
			&color_blending,
			nullptr,
			pipeline_layout,
			*render_pass,
			0,
			nullptr,
			-1);

	graphics_pipeline = device->createGraphicsPipelineUnique(nullptr, pipeline_ci).value;
}

void VulkanApplication::create_sync_objects() {
	frame_semaphores.resize(frames.size());

	for (auto &fs : frame_semaphores) {
		VkSemaphoreCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};
		fs.image_aquired = device->createSemaphoreUnique(vk::SemaphoreCreateInfo());
		fs.render_complete = device->createSemaphoreUnique(vk::SemaphoreCreateInfo());
	}
}

//
//	Scene
//
void VulkanApplication::create_mesh_buffer() {
	VkDeviceSize vertex_size = idx_offset = sizeof(vertices[0]) * uint32_t(vertices.size());
	VkDeviceSize index_size = sizeof(indices[0]) * indices.size();

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(*device, physical_device, vertex_size + index_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	void *data;
	// Copy vtx data
	vkMapMemory(*device, staging_buffer_memory, 0, vertex_size, 0, &data);
	memcpy(data, vertices.data(), size_t(vertex_size));
	vkUnmapMemory(*device, staging_buffer_memory);
	// Copy idx data
	vkMapMemory(*device, staging_buffer_memory, idx_offset, index_size, 0, &data);
	memcpy(data, indices.data(), size_t(index_size));
	vkUnmapMemory(*device, staging_buffer_memory);

	create_buffer(*device, physical_device, vertex_size + index_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh_buffer, mesh_buffer_memory);
	copy_buffer(staging_buffer, mesh_buffer, vertex_size + index_size);

	vkDestroyBuffer(*device, staging_buffer, nullptr);
	vkFreeMemory(*device, staging_buffer_memory, nullptr);
}

void VulkanApplication::create_descriptors() {
	{
		vk::DescriptorSetLayoutBinding ubo(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);
		vk::DescriptorSetLayoutBinding sampler(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr);
		vk::DescriptorSetLayoutBinding frag(2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment, nullptr);
		std::array<vk::DescriptorSetLayoutBinding, 3> bindings = { ubo, sampler, frag };

		descriptor_set_layout = device->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({}, bindings));
	}
	{
		vk::DescriptorPoolSize a(vk::DescriptorType::eUniformBuffer, frames.size());
		vk::DescriptorPoolSize b(vk::DescriptorType::eCombinedImageSampler, frames.size());
		std::array<vk::DescriptorPoolSize, 2> pool_sizes = { a, b };
		descriptor_pool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({}, frames.size(), pool_sizes));
	}
}

//
//	Texture
//
void VulkanApplication::create_texture_image() {
	// Load image
	SDL_Surface *surf = IMG_Load("resources/textures/viking_room.png");
	surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ABGR8888, 0);

	if (surf == nullptr) {
		SDL_LogError(0, "%s", SDL_GetError());
		throw std::runtime_error("Unable to create texture");
	}

	vk::Format format(get_vk_format(surf->format));
	create_image(surf->w, surf->h, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image, texture_image_memory);

	// Create data transfer buffer
	VkDeviceSize img_size = surf->w * surf->h * surf->format->BytesPerPixel;
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(*device, physical_device, img_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	// Copy image data to buffer
	void *data;
	vkMapMemory(*device, staging_buffer_memory, 0, img_size, 0, &data);
	memcpy(data, surf->pixels, img_size);
	vkUnmapMemory(*device, staging_buffer_memory);

	// Copy the buffer to the image and send it to graphics queue
	auto transfer_cbuffer = begin_transfer_command_buffer();
	transition_image_layout(transfer_cbuffer, texture_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copy_buffer_to_image(transfer_cbuffer, staging_buffer, texture_image, surf->w, surf->h);
	transition_image_layout(transfer_cbuffer, texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
	vkFreeCommandBuffers(*device, *transfer_command_pool, 1, &transfer_cbuffer);
	vkQueueWaitIdle(graphics_queue);
	vkFreeCommandBuffers(*device, *graphics_command_pool, 1, &receive_cbuffer);

	SDL_FreeSurface(surf);
	vkDestroyBuffer(*device, staging_buffer, nullptr);
	vkFreeMemory(*device, staging_buffer_memory, nullptr);
}

void VulkanApplication::create_image(uint32_t w, uint32_t h, vk::Format fmt, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory) {
	// Create VkImage

	vk::ImageCreateInfo ci({},
			vk::ImageType::e2D,
			fmt,
			vk::Extent3D(w, h, 1),
			1,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling(tiling),
			vk::ImageUsageFlags(usage),
			vk::SharingMode::eExclusive,
			nullptr,
			vk::ImageLayout::eUndefined);

	image = device->createImage(ci);

	/*
	 * VkImageCreateInfo ci{
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
	 if (vkCreateImage(*device, &ci, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create texture");
	}
	 */

	// Allocate VkImage memory
	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(*device, image, &mem_reqs);
	VkMemoryAllocateInfo ai{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = find_memory_type(physical_device, mem_reqs.memoryTypeBits, properties),
	};

	if (vkAllocateMemory(*device, &ai, nullptr, &image_memory) != VK_SUCCESS) {
		throw std::runtime_error("Unable to allocate memory for texture");
	}

	vkBindImageMemory(*device, image, image_memory, 0);
}

void VulkanApplication::transition_image_layout(VkCommandBuffer c_buffer, VkImage image, VkImageLayout from, VkImageLayout to) {
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
	texture_image_view = create_image_view(*device, texture_image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
}

void VulkanApplication::create_texture_sampler() {
	texture_sampler = device->createSamplerUnique(vk::SamplerCreateInfo(
			{}, vk::Filter::eLinear, vk::Filter::eLinear,
			vk::SamplerMipmapMode::eNearest,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			0,
			true, physical_device.getProperties().limits.maxSamplerAnisotropy,
			false, vk::CompareOp::eAlways, 0, 0,
			vk::BorderColor::eIntOpaqueBlack, false));
}

//
// Depth
//
void VulkanApplication::create_depth_resources() {
	vk::Format depth_format = find_depth_format();

	create_image(swapchain_extent.width, swapchain_extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image, depth_image_memory);
	depth_image_view = create_image_view(*device, depth_image, depth_format, vk::ImageAspectFlagBits::eDepth);
}

vk::Format VulkanApplication::find_supported_format(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
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

vk::Format VulkanApplication::find_depth_format() {
	return find_supported_format(
			{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

//
//	Imgui
//
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
		check_vk_result(vkCreateDescriptorPool(*device, &pool_info, nullptr, &imgui_descriptor_pool));
	}
	{ // Init vulkan
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = *instance;
		init_info.PhysicalDevice = physical_device;
		init_info.Device = *device;
		init_info.QueueFamily = queue_family_indices.graphics_family.value();
		init_info.Queue = graphics_queue;
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = imgui_descriptor_pool;
		init_info.Allocator = nullptr;
		init_info.MinImageCount = 2;
		init_info.ImageCount = uint32_t(frames.size());
		init_info.CheckVkResultFn = check_vk_result;

		ImGui_ImplVulkan_Init(&init_info, *render_pass);
	}
	{
		frames[0].command_buffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr));
		ImGui_ImplVulkan_CreateFontsTexture(frames[0].command_buffer.get());
		frames[0].command_buffer->end();

		vk::SubmitInfo submit(nullptr, nullptr, frames[0].command_buffer.get(), nullptr);
		graphics_queue.submit(submit, nullptr);
		graphics_queue.waitIdle();
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
}

void VulkanApplication::cleanup_imgui_context() {
	ImGui_ImplVulkan_Shutdown();
	vkDestroyDescriptorPool(*device, imgui_descriptor_pool, nullptr);
}

void VulkanApplication::draw_ui(VulkanFrame &frame) {
	ImGui::Render();
	auto data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(data, frame.command_buffer.get());
}

//
// Misc buffer fnc
//

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
	vkFreeCommandBuffers(*device, *transfer_command_pool, 1, &send_cbuffer);

	SDL_Log("Buffer copied!");
}

/*
void VulkanApplication::create_command_buffer(VkCommandPool pool, VkCommandBuffer *c_buffer) {
	VkCommandBufferAllocateInfo ai{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	check_vk_result(vkAllocateCommandBuffers(device, &ai, c_buffer));
}
*/

// Todo : wrapper class for command buffers

VkCommandBuffer VulkanApplication::begin_graphics_command_buffer() {
	VkCommandBufferAllocateInfo ai{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = *graphics_command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer c_buffer;
	vkAllocateCommandBuffers(*device, &ai, &c_buffer);

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
		.commandPool = *transfer_command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer c_buffer;
	vkAllocateCommandBuffers(*device, &ai, &c_buffer);

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
