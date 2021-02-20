#include "VulkanApplication.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
VulkanApplication::VulkanApplication() {
	// SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
	window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);

	// IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForVulkan(window);

	init_vulkan();

	SDL_ShowWindow(window);
}
VulkanApplication::~VulkanApplication() {
	SDL_Log("Cleaning up...");
	SDL_HideWindow(window);

	Input::cleanup();

	scene.reset();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	// SDL
	SDL_DestroyWindow(window);
	SDL_Log("Cleanup done! Bye bye!");
	IMG_Quit();
	SDL_Quit();
}
void VulkanApplication::run() {
	scene = std::make_unique<Scene>(this);
	load();
	SDL_Log("Resources loaded");
	scene->init();
	post_swapchain_init();

	SDL_Log("Init done, starting main loop...");
	main_loop();
	SDL_Log("Closing...");
}
void VulkanApplication::init_vulkan() {
	// Init dispatcher
	auto vkGetInstanceProcAddr = dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
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
		if (SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(*instance), &surf) != SDL_TRUE) {
			throw std::runtime_error("Unable to create window surface");
		}
		vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderDynamic> surface_deleter(*instance);
		surface = vk::UniqueSurfaceKHR(surf, surface_deleter);
		SDL_Log("Surface created!");
	}
	// Devices
	pick_physical_device();
	SDL_Log("Using %s", (char *)(physical_device.getProperties().deviceName));
	create_logical_device();
	SDL_Log("Logical device created!");

	// Command Pools
	graphics_command_pool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_family_indices.graphics_family.value()));
	transfer_command_pool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient, queue_family_indices.transfer_family.value()));
	SDL_Log("Command Pools created!");

	// Texture
	create_texture_sampler();
	SDL_Log("Texture sampler created!");
}
void VulkanApplication::post_swapchain_init(bool update) {
	device->waitIdle();
	swapchain.reset();
	create_swapchain();

	create_ui_renderpass();
	create_raster_renderpass();
	if (rtx) {
		init_rtx();
		raytracing_pipeline.reset();
		build_rtx_pipeline();
		create_rtx_SBT();
	} else {
		raster_pipe.reset();
		build_raster_pipeline(update);
	}
	if (update) {
		ImGui_ImplVulkan_Shutdown();
	}

	create_ui_context();
	create_frames();
	create_semaphores();
}
void VulkanApplication::main_loop() {
	bool run = true;
	int last_time = SDL_GetTicks();
	float delta_time;

	Input::start();
	start();

	while (run) {
		// Wait for V_Sync
		int time = SDL_GetTicks();
		delta_time = float(time - last_time) / 1000.0f;
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

		// Don't draw if the app is minimized
		if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)) {
			draw_frame();
		}
		device->waitIdle();
	}
}
void VulkanApplication::raster_scene(VulkanFrame &frame) {
	raster_pipe->bind_push_constants(*frame.command_buffer, 0, &scene->push_constants);
	raster_pipe->bind_descriptors(*frame.command_buffer);
	raster_pipe->bind(*frame.command_buffer);
	scene->draw(*frame.command_buffer);
}
void VulkanApplication::draw_ui(VulkanFrame &frame) {
	// UI
	ImGui::Render();
	auto imgui_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(imgui_data, frame.command_buffer.get());
}
void VulkanApplication::draw_frame() {
	vk::Semaphore image_acquired_semaphore = *frame_semaphores[semaphore_index].image_aquired;
	vk::Semaphore render_complete_semaphore = *frame_semaphores[semaphore_index].render_complete;

	// Acquire next image
	uint32_t image_id;
	vk::Result result;
	try {
		// Get next image and trigger image_acquired_semaphore once ready
		auto ret = device->acquireNextImageKHR(*swapchain, UINT64_MAX, image_acquired_semaphore, nullptr);
		image_id = ret.value;
		result = ret.result;
	} catch (vk::OutOfDateKHRError &error) {
	} // Handled below
	if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
		post_swapchain_init(true);
		return;
	}

	// Wait for the frame to be finished
	auto frame = &frames[image_id];
	if (device->waitForFences(frame->fence.get(), true, UINT64_MAX) != vk::Result::eSuccess) {
		throw std::runtime_error("Waiting for frame timed out!");
	}
	device->resetFences(frame->fence.get());

	frame->command_buffer->begin(vk::CommandBufferBeginInfo({}, nullptr));
	if (rtx) {
		debug_begin_label(frame->command_buffer.get(), "RTX");
		raytrace(*frame, image_id);
		debug_end_label(*frame->command_buffer);

		frame->begin_render_pass();
		debug_begin_label(frame->command_buffer.get(), "UI");
		draw_ui(*frame);
		debug_end_label(*frame->command_buffer);
		frame->end_render_pass();
	} else {
		frame->begin_render_pass();

		debug_begin_label(frame->command_buffer.get(), "Scene");
		raster_scene(*frame);
		debug_end_label(*frame->command_buffer);

		debug_begin_label(frame->command_buffer.get(), "UI");
		draw_ui(*frame);
		debug_end_label(*frame->command_buffer);
		frame->end_render_pass();
	}
	frame->command_buffer->end();

	// Submit Queue
	vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	vk::SubmitInfo si(image_acquired_semaphore, flags, frame->command_buffer.get(), render_complete_semaphore);
	graphics_queue.submit(si, frame->fence.get());

	// Present
	try {
		result = present_queue.presentKHR(vk::PresentInfoKHR(render_complete_semaphore, *swapchain, image_id, {}));
	} catch (vk::OutOfDateKHRError &error) {
	} // Handled below
	if (framebuffer_rezised || result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
		framebuffer_rezised = false;
		post_swapchain_init(true);
	}

	semaphore_index = (semaphore_index + 1) % swapchain_image_count;
}

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
	SDL_Log("%d devices found", uint32_t(devices.size()));
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
		vk::DeviceQueueCreateInfo ci({}, queue_family, 1, &queue_priority);
		req_queues.emplace_back(ci);
	}

	vk::PhysicalDeviceFeatures2 features2{};
	features2.features.samplerAnisotropy = true;
	features2.features.shaderInt64 = true;
	vk::PhysicalDeviceBufferDeviceAddressFeatures device_address{};
	device_address.bufferDeviceAddress = true;
	vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipeline{};
	rt_pipeline.rayTracingPipeline = true;
	vk::PhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structures{};
	acceleration_structures.accelerationStructure = true;
	vk::PhysicalDeviceDescriptorIndexingFeatures descriptor_indexing{};
	descriptor_indexing.runtimeDescriptorArray = true;
	descriptor_indexing.shaderStorageBufferArrayNonUniformIndexing = true;
	vk::PhysicalDeviceRayQueryFeaturesKHR ray_query{};
	ray_query.rayQuery = true;

	vk::StructureChain requested_features = {
		features2,
		device_address,
		rt_pipeline,
		acceleration_structures,
		descriptor_indexing,
		ray_query
	};

	vk::DeviceCreateInfo ci({}, req_queues, req_validation_layers, req_device_extensions, nullptr);
	ci.setPNext(&requested_features.get<vk::PhysicalDeviceFeatures2>());
	device = physical_device.createDeviceUnique(ci);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

	graphics_queue = device->getQueue(queue_family_indices.graphics_family.value(), 0);
	present_queue = device->getQueue(queue_family_indices.present_family.value(), 0);
	transfer_queue = device->getQueue(queue_family_indices.transfer_family.value(), 0);
}
void VulkanApplication::create_texture_sampler() {
}

void VulkanApplication::create_swapchain() {
	auto capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);
	auto surface_fmt = choose_surface_format(physical_device, *surface);
	auto present_mode = choose_present_mode(physical_device, *surface);
	swapchain_extent = choose_extent(capabilities, window);
	uint32_t req_image_count = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && req_image_count > capabilities.maxImageCount) {
		req_image_count = capabilities.maxImageCount;
	}
	auto family_indices = find_queue_families(physical_device, *surface);
	std::array<const uint32_t, 2> queue_indices({ family_indices.graphics_family.value(), family_indices.present_family.value() });

	vk::SwapchainCreateInfoKHR ci({},
			*surface,
			req_image_count,
			surface_fmt.format,
			surface_fmt.colorSpace,
			swapchain_extent,
			1,
			{ vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst },
			vk::SharingMode::eExclusive,
			queue_indices,
			capabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			present_mode,
			VK_TRUE);

	swapchain = device->createSwapchainKHRUnique(ci);
	swapchain_format = surface_fmt.format;
	swapchain_images = device->getSwapchainImagesKHR(*swapchain);
	swapchain_image_count = swapchain_images.size();
}
void VulkanApplication::build_raster_pipeline(bool update) {
	depth_image = std::unique_ptr<Image>(new Image(*this, swapchain_extent.width, swapchain_extent.height, find_depth_format(), vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth));

	raster_pipe = std::make_unique<GraphicsPipeline>();

	scene->write_descriptors(*raster_pipe);
	raster_pipe->build_pipeline(*device, swapchain_extent, *raster_render_pass);
}
void VulkanApplication::create_raster_renderpass() {
	// Render pass
	std::vector<vk::AttachmentDescription> attachments;
	vk::AttachmentDescription attachment(
			{},
			swapchain_format,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR);
	attachments.push_back(attachment);

	vk::AttachmentDescription depth(
			{},
			find_depth_format(),
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal);
	attachments.push_back(depth);

	vk::AttachmentReference color_ref(0U, vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depth_ref(1U, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::SubpassDependency dependency(
			VK_SUBPASS_EXTERNAL,
			0,
			{ vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests },
			{ vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests },
			{},
			{ vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite },
			{});
	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, nullptr, color_ref, nullptr, &depth_ref, nullptr);

	raster_render_pass = device->createRenderPassUnique(vk::RenderPassCreateInfo({}, attachments, subpass, dependency));
}
void VulkanApplication::create_ui_renderpass() {
	std::vector<vk::AttachmentDescription> attachments;
	vk::AttachmentDescription attachment(
			{},
			swapchain_format,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eLoad,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eGeneral,
			vk::ImageLayout::ePresentSrcKHR);
	attachments.push_back(attachment);

	vk::AttachmentReference color_ref(0U, vk::ImageLayout::eColorAttachmentOptimal);
	vk::SubpassDependency dependency(
			VK_SUBPASS_EXTERNAL,
			0,
			{ vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests },
			{ vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests },
			{},
			{ vk::AccessFlagBits::eColorAttachmentWrite },
			{});
	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, nullptr, color_ref, nullptr, nullptr, nullptr);

	ui_render_pass = device->createRenderPassUnique(vk::RenderPassCreateInfo({}, attachments, subpass, dependency));
}

void VulkanApplication::create_ui_context() {
	static std::array<vk::DescriptorPoolSize, 11> pool_sizes{};
	pool_sizes[0].type = vk::DescriptorType::eSampler;
	pool_sizes[0].descriptorCount = 1000;
	pool_sizes[1].type = vk::DescriptorType::eCombinedImageSampler;
	pool_sizes[1].descriptorCount = 1000;
	pool_sizes[2].type = vk::DescriptorType::eSampledImage;
	pool_sizes[2].descriptorCount = 1000;
	pool_sizes[3].type = vk::DescriptorType::eStorageImage;
	pool_sizes[3].descriptorCount = 1000;
	pool_sizes[4].type = vk::DescriptorType::eUniformTexelBuffer;
	pool_sizes[4].descriptorCount = 1000;
	pool_sizes[5].type = vk::DescriptorType::eStorageTexelBuffer;
	pool_sizes[5].descriptorCount = 1000;
	pool_sizes[6].type = vk::DescriptorType::eUniformBuffer;
	pool_sizes[6].descriptorCount = 1000;
	pool_sizes[7].type = vk::DescriptorType::eStorageBuffer;
	pool_sizes[7].descriptorCount = 1000;
	pool_sizes[8].type = vk::DescriptorType::eUniformBufferDynamic;
	pool_sizes[8].descriptorCount = 1000;
	pool_sizes[9].type = vk::DescriptorType::eStorageBufferDynamic;
	pool_sizes[9].descriptorCount = 1000;
	pool_sizes[10].type = vk::DescriptorType::eInputAttachment;
	pool_sizes[10].descriptorCount = 1000;

	imgui_descriptor_pool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet }, 1000 * uint32_t(pool_sizes.size()), pool_sizes));

	ImGui_ImplVulkan_InitInfo imgui_init_info{};
	imgui_init_info.Instance = *instance;
	imgui_init_info.PhysicalDevice = physical_device;
	imgui_init_info.Device = *device;
	imgui_init_info.QueueFamily = queue_family_indices.graphics_family.value();
	imgui_init_info.Queue = graphics_queue;
	imgui_init_info.PipelineCache = VK_NULL_HANDLE;
	imgui_init_info.DescriptorPool = *imgui_descriptor_pool;
	imgui_init_info.Allocator = nullptr;
	imgui_init_info.MinImageCount = 2;
	imgui_init_info.ImageCount = uint32_t(swapchain_image_count);
	imgui_init_info.CheckVkResultFn = nullptr;
	imgui_init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	if (!rtx)
		ImGui_ImplVulkan_Init(&imgui_init_info, *raster_render_pass);
	else
		ImGui_ImplVulkan_Init(&imgui_init_info, *ui_render_pass);

	auto cmd_buf = create_commandbuffer();
	ImGui_ImplVulkan_CreateFontsTexture(cmd_buf);
	flush_commandbuffer(cmd_buf);

	ImGui_ImplVulkan_DestroyFontUploadObjects();
}
void VulkanApplication::create_frames() {
	frames.resize(swapchain_image_count);
	for (uint32_t i = 0; i < frames.size(); i++) {
		frames[i].init_frame(*device);
		frames[i].raster_image_view = create_image_view(*device, swapchain_images[i], swapchain_format, vk::ImageAspectFlagBits::eColor);
		if (!rtx)
			frames[i].create_framebuffer(swapchain_extent, *raster_render_pass, depth_image->image_view);
		else
			frames[i].create_framebuffer(swapchain_extent, *ui_render_pass);
		frames[i].create_command_buffers(*graphics_command_pool);
		frames[i].create_sync_objects();
	}
}
void VulkanApplication::create_semaphores() {
	frame_semaphores.resize(swapchain_image_count);

	for (auto &fs : frame_semaphores) {
		VkSemaphoreCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};
		fs.image_aquired = device->createSemaphoreUnique(vk::SemaphoreCreateInfo());
		fs.render_complete = device->createSemaphoreUnique(vk::SemaphoreCreateInfo());
	}
}

void VulkanApplication::init_rtx() {
	auto properties = physical_device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
	rtx_properties = properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
	if (rtx_properties.maxRayRecursionDepth <= 1) {
		SDL_LogError(0, "Device doesn't support ray recursion!");
		throw std::runtime_error("Device doesn't support ray recursion!");
	}
}
void VulkanApplication::build_rtx_pipeline(bool update) {
	// Allocate render image TODO : recreate on swapchain recreation, link to swapchain??
	rtx_result_image = std::unique_ptr<Image>(new Image(
			*device, physical_device,
			swapchain_extent.width, swapchain_extent.height,
			swapchain_format, vk::ImageTiling::eOptimal,
			{ vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage },
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::ImageAspectFlagBits::eColor));
	debug_name_object(*device, uint64_t(VkImage(rtx_result_image->image)), rtx_result_image->image.objectType, "RTX Result Image");

	raytracing_pipeline = std::make_unique<RaytracingPipeline>();

	// Binding 0 ; Storage Image
	vk::DescriptorImageInfo image_info({}, rtx_result_image->image_view, vk::ImageLayout::eGeneral);
	raytracing_pipeline->add_descriptor(vk::DescriptorType::eStorageImage, image_info, 0, vk::ShaderStageFlagBits::eRaygenKHR, 1, 1);

	scene->write_descriptors(*raytracing_pipeline);

	raytracing_pipeline->build_pipeline(*device);
}
void VulkanApplication::create_rtx_SBT() {
	uint32_t shader_amount = raytracing_pipeline->shader_groups.size();
	const uint32_t group_size_aligned = (rtx_properties.shaderGroupHandleSize + rtx_properties.shaderGroupBaseAlignment - 1) & ~(rtx_properties.shaderGroupBaseAlignment - 1);
	const uint32_t shader_binding_table_size = group_size_aligned * shader_amount;

	shader_binding_table = std::unique_ptr<Buffer>(new Buffer(*device, physical_device, shader_binding_table_size, { vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress }, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible }));

	std::vector<uint8_t> shader_handle_storage = device->getRayTracingShaderGroupHandlesKHR<uint8_t>(raytracing_pipeline->get_pipeline(), 0, shader_amount, shader_binding_table_size);

	void *mapped = device->mapMemory(shader_binding_table->memory, 0, shader_binding_table_size);
	auto *data = reinterpret_cast<uint8_t *>(mapped);
	for (uint32_t i = 0; i < shader_amount; i++) {
		memcpy(data, shader_handle_storage.data() + i * rtx_properties.shaderGroupHandleSize, rtx_properties.shaderGroupHandleSize);
		data += group_size_aligned;
	}
	device->unmapMemory(shader_binding_table->memory);
}
void VulkanApplication::raytrace(VulkanFrame &frame, int image_id) {
	vk::ImageMemoryBarrier image_barrier({}, vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, rtx_result_image->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
	frame.command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, image_barrier);

	raytracing_pipeline->bind(*frame.command_buffer);
	raytracing_pipeline->bind_descriptors(*frame.command_buffer);
	raytracing_pipeline->bind_push_constants(*frame.command_buffer, 0, &scene->push_constants);

	const uint32_t group_size = (rtx_properties.shaderGroupHandleSize + rtx_properties.shaderGroupBaseAlignment - 1) & ~(rtx_properties.shaderGroupBaseAlignment - 1);
	std::vector<vk::StridedDeviceAddressRegionKHR> strides{
		vk::StridedDeviceAddressRegionKHR(shader_binding_table->address + 0u * group_size, group_size, group_size), // Rgen
		vk::StridedDeviceAddressRegionKHR(shader_binding_table->address + 1u * group_size, group_size, group_size), // Miss
		vk::StridedDeviceAddressRegionKHR(shader_binding_table->address + 3u * group_size, group_size, group_size), // Hit
		vk::StridedDeviceAddressRegionKHR(0u, 0u, 0u),
	};
	frame.command_buffer->traceRaysKHR(strides[0], strides[1], strides[2], strides[3], swapchain_extent.width, swapchain_extent.height, 1);

	image_barrier.image = swapchain_images[image_id];
	image_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
	image_barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
	image_barrier.oldLayout = vk::ImageLayout::eUndefined;
	image_barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
	frame.command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, image_barrier);

	image_barrier.image = rtx_result_image->image;
	image_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
	image_barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
	image_barrier.oldLayout = vk::ImageLayout::eGeneral;
	image_barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
	frame.command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, image_barrier);

	vk::ImageCopy copy_region{};
	copy_region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copy_region.srcOffset = vk::Offset3D(0, 0, 0);
	copy_region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copy_region.dstOffset = vk::Offset3D(0, 0, 0);
	copy_region.extent = vk::Extent3D(swapchain_extent, 1);
	frame.command_buffer->copyImage(rtx_result_image->image, vk::ImageLayout::eTransferSrcOptimal, swapchain_images[image_id], vk::ImageLayout::eTransferDstOptimal, copy_region);

	image_barrier.image = swapchain_images[image_id];
	image_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	image_barrier.dstAccessMask = vk::AccessFlagBits::eIndexRead;
	image_barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	image_barrier.newLayout = vk::ImageLayout::eGeneral;
	frame.command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eVertexInput, {}, nullptr, nullptr, image_barrier);
}

void VulkanApplication::toggle_rtx() {
	rtx = !rtx;
	post_swapchain_init(true);
}

void VulkanApplication::copy_buffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size) {
	auto send_cbuffer = std::move(device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(*transfer_command_pool, vk::CommandBufferLevel::ePrimary, 1)).front());
	send_cbuffer->begin(vk::CommandBufferBeginInfo({}, nullptr));
	vk::BufferCopy region(0, 0, size);
	send_cbuffer->copyBuffer(src, dst, region);

	vk::BufferMemoryBarrier barrier(vk::AccessFlagBits::eTransferWrite, {}, queue_family_indices.transfer_family.value(), queue_family_indices.graphics_family.value(), dst, 0, size);
	send_cbuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, {}, nullptr, barrier, nullptr);
	send_cbuffer->end();
	transfer_queue.submit(vk::SubmitInfo(nullptr, {}, *send_cbuffer, nullptr), nullptr);

	auto receive_cbuffer = std::move(device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(*graphics_command_pool, vk::CommandBufferLevel::ePrimary, 1)).front());
	receive_cbuffer->begin(vk::CommandBufferBeginInfo({}, nullptr));

	// reuse the old barrier create info
	barrier.setSrcAccessMask({});
	barrier.setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead);

	receive_cbuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eVertexInput, {}, nullptr, barrier, nullptr);
	receive_cbuffer->end();
	graphics_queue.submit(vk::SubmitInfo(nullptr, {}, *receive_cbuffer, nullptr));

	transfer_queue.waitIdle();
	graphics_queue.waitIdle();
}
bool VulkanApplication::check_validation_layer_support() {
	auto available_layers = vk::enumerateInstanceLayerProperties();

	for (const char *layer_name : req_validation_layers) {
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
std::vector<const char *> VulkanApplication::get_required_extensions(SDL_Window *window) {
	// Get extension count & names
	uint32_t sdl_extension_count = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, nullptr);
	std::vector<const char *> required_extension_names = {};
	required_extension_names.resize(sdl_extension_count);
	SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, required_extension_names.data());

	if (enable_validation_layers) {
		required_extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	SDL_Log("Required instance extensions:");
	for (const auto &extension : required_extension_names) {
		SDL_Log("\t %s", extension);
	}

	return required_extension_names;
}

vk::CommandBuffer VulkanApplication::create_commandbuffer(vk::QueueFlagBits type, bool begin) {
	vk::CommandBufferAllocateInfo alloc_info{};
	alloc_info.commandBufferCount = 1;
	alloc_info.level = vk::CommandBufferLevel::ePrimary;

	switch (type) {
		case vk::QueueFlagBits::eGraphics:
			alloc_info.commandPool = *graphics_command_pool;
			break;

		case vk::QueueFlagBits::eTransfer:
			alloc_info.commandPool = *transfer_command_pool;
			break;
		default:
			SDL_LogError(0, "Queue type not supported");
			throw std::runtime_error("Queue type not supported");
			break;
	}

	auto cmd = device->allocateCommandBuffers(alloc_info).front();
	if (begin) {
		cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr));
	}
	return cmd;
}
std::vector<vk::CommandBuffer> VulkanApplication::create_commandbuffers(uint32_t count, vk::QueueFlagBits type, bool begin) {
	vk::CommandBufferAllocateInfo alloc_info{};
	alloc_info.commandBufferCount = count;
	alloc_info.level = vk::CommandBufferLevel::ePrimary;

	switch (type) {
		case vk::QueueFlagBits::eGraphics:
			alloc_info.commandPool = *graphics_command_pool;
			break;

		case vk::QueueFlagBits::eTransfer:
			alloc_info.commandPool = *transfer_command_pool;
			break;
		default:
			SDL_LogError(0, "Queue type not supported");
			throw std::runtime_error("Queue type not supported");
			break;
	}

	auto cmd = device->allocateCommandBuffers(alloc_info);
	if (begin) {
		for (auto &c : cmd)
			c.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr));
	}
	return cmd;
}
void VulkanApplication::flush_commandbuffer(vk::CommandBuffer cmd, vk::QueueFlagBits type, bool wait) {
	cmd.end();
	vk::CommandPool command_pool;
	vk::Queue queue;

	switch (type) {
		case vk::QueueFlagBits::eGraphics:
			command_pool = *graphics_command_pool;
			queue = graphics_queue;
			break;

		case vk::QueueFlagBits::eTransfer:
			command_pool = *transfer_command_pool;
			queue = transfer_queue;
			break;
		default:
			SDL_LogWarn(0, "Queue type not supported");
			break;
	}
	queue.submit(vk::SubmitInfo(nullptr, nullptr, cmd, nullptr), nullptr);

	if (wait) {
		queue.waitIdle();
	}
	device->freeCommandBuffers(command_pool, cmd);
}
void VulkanApplication::flush_commandbuffers(std::vector<vk::CommandBuffer> cmd, vk::QueueFlagBits type, bool wait) {
	for (auto &c : cmd) {
		c.end();
	}

	vk::CommandPool command_pool;
	vk::Queue queue;
	switch (type) {
		case vk::QueueFlagBits::eGraphics:
			command_pool = *graphics_command_pool;
			queue = graphics_queue;
			break;

		case vk::QueueFlagBits::eTransfer:
			command_pool = *transfer_command_pool;
			queue = transfer_queue;
			break;
		default:
			SDL_LogWarn(0, "Queue type not supported");
			break;
	}
	queue.submit(vk::SubmitInfo(nullptr, nullptr, cmd, nullptr), nullptr);
	if (wait) {
		queue.waitIdle();
	}
	device->freeCommandBuffers(command_pool, cmd);
}

float VulkanApplication::get_aspect_ratio() const {
	auto a = swapchain_extent;
	return float(a.width) / float(a.height);
}
void VulkanApplication::refresh_shaders() {
	build_raster_pipeline(true); // TODO : Do this automatically when changes are detected in the shaders?
	if (rtx) {
		build_rtx_pipeline(true);
		create_rtx_SBT();
	}
}
int VulkanApplication::get_graphics_family_index() const {
	return queue_family_indices.graphics_family.value();
}
int VulkanApplication::get_transfer_family_index() const {
	return queue_family_indices.transfer_family.value();
}
vk::Format VulkanApplication::find_depth_format() {
	return find_supported_format(physical_device,
			{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}
vk::SurfaceFormatKHR VulkanApplication::choose_surface_format(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
	/*
	for (const auto &fmt : physical_device.getSurfaceFormatsKHR(surface)) {
		if (fmt.format == vk::Format::eB8G8R8A8Unorm && fmt.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
			return fmt;
		}
	}
	*/
	return physical_device.getSurfaceFormatsKHR(surface).front();
}
vk::PresentModeKHR VulkanApplication::choose_present_mode(const vk::PhysicalDevice physical_device, const vk::SurfaceKHR surface) {
	for (const auto &mode : physical_device.getSurfacePresentModesKHR(surface)) {
		if (mode == vk::PresentModeKHR::eMailbox) {
			return mode;
		}
	}
	SDL_Log("Present mode: VK_PRESENT_MODE_FIFO_KHR");
	return vk::PresentModeKHR::eFifo;
}
vk::Extent2D VulkanApplication::choose_extent(const vk::SurfaceCapabilitiesKHR &capabilities, SDL_Window *window) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		int w, h;
		SDL_Vulkan_GetDrawableSize(window, &w, &h);
		vk::Extent2D ext = { uint32_t(w), uint32_t(h) };
		ext.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, swapchain_extent.width));
		ext.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, swapchain_extent.height));
		return ext;
	}
}
