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
	init_rtx();
}
VulkanApplication::~VulkanApplication() {
	SDL_Log("Cleaning up...");
	SDL_HideWindow(window);

	meshes.clear();

	Input::cleanup();

	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	// SDL
	SDL_DestroyWindow(window);
	SDL_Log("Cleanup done! Bye bye!");
	IMG_Quit();
	SDL_Quit();
}
void VulkanApplication::run() {
	load(meshes);
	for (auto &mesh : meshes) {
		mesh->load(this);
	}
	SDL_Log("Resources loaded");

	build_BLAS({ vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction });
	SDL_Log("BLAS created");
	build_TLAS();
	SDL_Log("TLAS created");
	build_rtx_pipeline();
	SDL_Log("RTX Pipeline created");
	create_rtx_SBT();
	SDL_Log("SBT Created");

	SDL_Log("Init done, starting main loop...");
	main_loop();
	SDL_Log("Closing...");
}
float VulkanApplication::get_aspect_ratio() const {
	auto a = swapchain->get_extent();
	return float(a.width) / float(a.height);
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
	create_texture_image();
	SDL_Log("Image created!");
	create_texture_sampler();
	SDL_Log("Texture sampler created!");

	SDL_Log("Image view created!");

	Swapchain::info = {
		.instance = *instance,
		.surface = *surface,
		.window = window,
		.physical_device = physical_device,
		.device = *device,
		.queue_family = queue_family_indices.graphics_family.value(),
		.queue = graphics_queue,
		.command_pool = *graphics_command_pool,
		.texture_sampler = *texture_sampler,
		.texture_image_view = texture->image_view,
	};
	swapchain = std::make_unique<Swapchain>();
	SDL_ShowWindow(window);
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
		ImGui::Render();

		// Don't draw if the app is minimized
		if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)) {
			draw_frame();
		}
		device->waitIdle();
	}
}
void VulkanApplication::draw_scene(VulkanFrame &frame) {
	// SCENE UBO
	frame.scene_buffer->write_data(&camera_matrices, sizeof(CameraMatrices));
	// FRAG UBO
	frame.scene_buffer->write_data(&fubo, sizeof(fubo), sizeof(CameraMatrices));

	frame.command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, swapchain->get_pipeline());
	frame.command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, swapchain->get_pipeline_layout(), 0, frame.scene_descriptor_set.get(), nullptr);

	for (auto &mesh : meshes) {
		frame.transform_buffer->write_data(&mesh->transform, sizeof(mesh->transform), mesh->id * sizeof(MeshUBO));
		frame.command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, swapchain->get_pipeline_layout(), 1, frame.mesh_descriptor_set.get(), mesh->id * sizeof(MeshUBO));
		mesh->draw(frame);
	}
}
void VulkanApplication::draw_ui(VulkanFrame &frame) {
	// UI
	ImGui::Render();
	auto imgui_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(imgui_data, frame.command_buffer.get());
}

void VulkanApplication::draw_frame() {
	// Get the image
	vk::Semaphore image_acquired_semaphore = swapchain->get_image_acquired_semaphore(semaphore_index);
	vk::Semaphore render_complete_semaphore = swapchain->get_render_complete_semaphore(semaphore_index);

	uint32_t image_id;
	{
		vk::Result result;
		try {
			// Get next image and trigger image_available once ready
			auto ret = device->acquireNextImageKHR(swapchain->get_swapchain(), UINT64_MAX, image_acquired_semaphore, nullptr);
			image_id = ret.value;
			result = ret.result;
		} catch (vk::OutOfDateKHRError &error) {
		} // Handled below

		if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
			device->waitIdle();
			swapchain.reset();
			swapchain = std::make_unique<Swapchain>();
			return;
		}
	}

	auto frame = swapchain->get_frame(image_id);

	// Wait for the frame to be finished
	if (device->waitForFences(frame->fence.get(), true, UINT64_MAX) != vk::Result::eSuccess) {
		throw std::runtime_error("Waiting for frame timed out!");
	}
	device->resetFences(frame->fence.get());

	// SCENE UBO
	frame->scene_buffer->write_data(&camera_matrices, sizeof(CameraMatrices));
	if (rtx) {
		raytrace(*frame, image_id);
	} else {
		frame->begin_render_pass(); // Todo : split this into 2 render passes
		draw_scene(*frame); // Render 3D objects in the scene
		draw_ui(*frame);
		frame->end_render_pass();
	}

	// Submit Queue
	{
		vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo si(image_acquired_semaphore, flags, frame->command_buffer.get(), render_complete_semaphore);
		graphics_queue.submit(si, frame->fence.get());
	}
	// Present
	{
		vk::SwapchainKHR swap = swapchain->get_swapchain();
		vk::Result result;
		try {
			result = present_queue.presentKHR(vk::PresentInfoKHR(render_complete_semaphore, swap, image_id, {}));
		} catch (vk::OutOfDateKHRError &error) {
		} // Handled below

		if (framebuffer_rezised || result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
			framebuffer_rezised = false;
			device->waitIdle();
			swapchain.reset();
			swapchain = std::make_unique<Swapchain>();
		}
	}
	semaphore_index = (semaphore_index + 1) % swapchain->get_image_count();
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

	auto supported_features = physical_device.getFeatures2<
			vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceBufferDeviceAddressFeatures,
			vk::PhysicalDeviceRayTracingPipelineFeaturesKHR, vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
	vk::DeviceCreateInfo ci({}, req_queues, req_validation_layers, req_device_extensions, nullptr);
	ci.setPNext(&supported_features.get<vk::PhysicalDeviceFeatures2>());
	device = physical_device.createDeviceUnique(ci);

	VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

	graphics_queue = device->getQueue(queue_family_indices.graphics_family.value(), 0);
	present_queue = device->getQueue(queue_family_indices.present_family.value(), 0);
	transfer_queue = device->getQueue(queue_family_indices.transfer_family.value(), 0);
}
void VulkanApplication::create_texture_image() {
	// Load image
	SDL_Surface *surf = IMG_Load("resources/textures/viking_room.png");
	surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ABGR8888, 0);

	if (surf == nullptr) {
		SDL_LogError(0, "%s", SDL_GetError());
		throw std::runtime_error("Unable to create texture");
	}

	vk::Format format(get_vk_format(surf->format));
	texture = std::unique_ptr<Image>(new Image(*device, physical_device, surf->w, surf->h, format, vk::ImageTiling::eOptimal, { vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled }, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor));

	// Create data transfer buffer
	vk::DeviceSize img_size = surf->w * surf->h * surf->format->BytesPerPixel;
	Buffer staging_buffer(*device, physical_device, img_size, { vk::BufferUsageFlagBits::eTransferSrc }, { vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent });
	staging_buffer.write_data(surf->pixels, img_size);

	// Copy the buffer to the image and send it to graphics queue
	vk::UniqueCommandBuffer transfer_cbuffer = std::move(device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(*transfer_command_pool, vk::CommandBufferLevel::ePrimary, 1)).front());
	transfer_cbuffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	texture->transition_layout(*transfer_cbuffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, queue_family_indices.transfer_family.value(), queue_family_indices.graphics_family.value());
	copy_buffer_to_image(*transfer_cbuffer, staging_buffer.buffer, texture->image, surf->w, surf->h);
	texture->transition_layout(*transfer_cbuffer, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, queue_family_indices.transfer_family.value(), queue_family_indices.graphics_family.value());
	transfer_cbuffer->end();
	transfer_queue.submit(vk::SubmitInfo(nullptr, nullptr, *transfer_cbuffer, nullptr));

	// Receive the image on the graphics queue
	vk::UniqueCommandBuffer graphics_cbuffer = std::move(device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(*graphics_command_pool, vk::CommandBufferLevel::ePrimary, 1)).front());
	graphics_cbuffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	vk::ImageMemoryBarrier barrier(
			{}, vk::AccessFlagBits::eVertexAttributeRead,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
			queue_family_indices.transfer_family.value(), queue_family_indices.graphics_family.value(),
			texture->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
	graphics_cbuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eVertexInput, {}, nullptr, nullptr, barrier);
	graphics_cbuffer->end();
	graphics_queue.submit(vk::SubmitInfo({}, {}, *graphics_cbuffer));

	// Wait for everything to process
	transfer_queue.waitIdle();
	graphics_queue.waitIdle();

	SDL_FreeSurface(surf);
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

///Copy a buffer from a staging buffer in the transfer queue to a buffer in the graphics queue transferring ownership.
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
		//required_extension_names.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
	}

	SDL_Log("Required instance extensions:");
	for (const auto &extension : required_extension_names) {
		SDL_Log("\t %s", extension);
	}

	return required_extension_names;
}

void VulkanApplication::init_rtx() {
	auto properties = physical_device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
	rtx_properties = properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
}
void VulkanApplication::build_BLAS(vk::BuildAccelerationStructureFlagsKHR flags) {
	std::vector<std::unique_ptr<AccelerationStructure>> orig_blas;
	bool do_compaction = (flags & vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction) == vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction;
	auto query_pool = device->createQueryPoolUnique(vk::QueryPoolCreateInfo({}, vk::QueryType::eAccelerationStructureCompactedSizeKHR, meshes.size(), {}));

	{
		std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> build_infos;
		vk::DeviceSize max_scratch = 0;

		// Create BLAS
		for (auto &mesh : meshes) {
			vk::AccelerationStructureBuildGeometryInfoKHR build_info(
					vk::AccelerationStructureTypeKHR::eBottomLevel,
					flags,
					vk::BuildAccelerationStructureModeKHR::eBuild,
					nullptr, nullptr,
					1, &mesh->geometry, nullptr,
					nullptr);
			auto size_info = device->getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build_info, mesh->get_primitive_count());
			max_scratch = std::max(max_scratch, size_info.buildScratchSize);
			orig_blas.emplace_back(std::make_unique<AccelerationStructure>(*device, physical_device, vk::AccelerationStructureTypeKHR::eBottomLevel, size_info.accelerationStructureSize));
			build_infos.emplace_back(build_info);
		}

		// Build BLAS
		std::vector<vk::CommandBuffer> cmd_buffs = device->allocateCommandBuffers(vk::CommandBufferAllocateInfo(*graphics_command_pool, vk::CommandBufferLevel::ePrimary, meshes.size()));
		Buffer scratch_buffer(*device, physical_device, max_scratch, { vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer }, vk::MemoryPropertyFlagBits::eDeviceLocal);
		for (int i = 0; i < meshes.size(); i++) {
			build_infos[i].dstAccelerationStructure = orig_blas[i]->get_acceleration_structure();
			build_infos[i].scratchData.deviceAddress = scratch_buffer.address;

			cmd_buffs[i].begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
			cmd_buffs[i].buildAccelerationStructuresKHR(build_infos[i], &meshes[i]->range_info);
			cmd_buffs[i].pipelineBarrier(
					vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
					vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {},
					vk::MemoryBarrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR, vk::AccessFlagBits::eAccelerationStructureReadKHR),
					nullptr, nullptr);

			if (do_compaction)
				cmd_buffs[i].writeAccelerationStructuresPropertiesKHR(orig_blas[i]->get_acceleration_structure(), vk::QueryType::eAccelerationStructureCompactedSizeKHR, *query_pool, i);
			cmd_buffs[i].end();
		}
		graphics_queue.submit(vk::SubmitInfo(nullptr, {}, cmd_buffs, nullptr));
		graphics_queue.waitIdle();
		device->freeCommandBuffers(*graphics_command_pool, cmd_buffs);
	}

	if (do_compaction) {
		// Copy them in the meshes while compressing them
		std::vector<vk::DeviceSize> compact_sizes = device->getQueryPoolResults<vk::DeviceSize>(*query_pool, 0, meshes.size(), meshes.size() * sizeof(vk::DeviceSize), sizeof(vk::DeviceSize), vk::QueryResultFlagBits::eWait).value;
		vk::UniqueCommandBuffer cmd_buf = std::move(device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(*graphics_command_pool, vk::CommandBufferLevel::ePrimary, 1)).front());
		cmd_buf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr));
		for (int i = 0; i < meshes.size(); i++) {
			meshes[i]->blas = std::make_unique<AccelerationStructure>(*device, physical_device, vk::AccelerationStructureTypeKHR::eBottomLevel, compact_sizes[i]);
			cmd_buf->copyAccelerationStructureKHR(vk::CopyAccelerationStructureInfoKHR(orig_blas[i]->get_acceleration_structure(), meshes[i]->blas->get_acceleration_structure(), vk::CopyAccelerationStructureModeKHR::eCompact));
		}
		cmd_buf->end();
		graphics_queue.submit(vk::SubmitInfo(nullptr, {}, *cmd_buf, nullptr));
		graphics_queue.waitIdle();
		/*
		SDL_Log("RTX Blas: reducing from %u to %u = %u (%2.2f%s smaller) ",
				stat_total_ori_size,
				stat_totat_compact_size,
				stat_total_ori_size - stat_totat_compact_size,
				(stat_total_ori_size - stat_totat_compact_size) / float(stat_total_ori_size) * 100.f, "%");
				*/
	} else {
		// Just move the BLASes into the meshes for future
		for (int i = 0; i < meshes.size(); i++) {
			meshes[i]->blas = std::move(orig_blas[i]);
		}
	}
}
void VulkanApplication::build_TLAS(bool update, vk::BuildAccelerationStructureFlagsKHR flags) {
	int hit_goup_id = 0;

	// Todo : multiple instances
	vk::AccelerationStructureInstanceKHR as_instance{};
	auto transform = meshes[0]->transform;
	transform = glm::transpose(transform);
	memcpy(&as_instance.transform, &transform, sizeof(transform));
	as_instance.flags = (unsigned int)(vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable);
	as_instance.instanceShaderBindingTableRecordOffset = hit_goup_id;
	as_instance.instanceCustomIndex = 0;
	as_instance.accelerationStructureReference = meshes[0]->blas->get_address();
	as_instance.mask = 0xFF;

	if (update) {
		rtx_instances_buffer.reset();
	}
	Buffer *temp = new Buffer(*device, physical_device, sizeof(vk::AccelerationStructureInstanceKHR), { vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR }, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible });
	rtx_instances_buffer = std::unique_ptr<Buffer>(temp);
	rtx_instances_buffer->write_data(&as_instance, sizeof(vk::AccelerationStructureInstanceKHR));
	debug_set_object_name(*device, (uint64_t)(VkBuffer)rtx_instances_buffer.get(), vk::DebugReportObjectTypeEXT::eBuffer, "RTX INSTANCES BUFFER");

	vk::AccelerationStructureGeometryKHR tlasGeo{};
	tlasGeo.geometryType = vk::GeometryTypeKHR::eInstances;
	tlasGeo.flags = vk::GeometryFlagBitsKHR::eOpaque;

	vk::DeviceOrHostAddressConstKHR addr(rtx_instances_buffer->address);
	vk::AccelerationStructureGeometryInstancesDataKHR geometry_instances_data(false, addr);
	tlasGeo.geometry.instances = geometry_instances_data;

	vk::AccelerationStructureBuildGeometryInfoKHR build_info{};
	build_info.flags = flags;
	build_info.geometryCount = 1;
	build_info.pGeometries = &tlasGeo;
	build_info.mode = update ? vk::BuildAccelerationStructureModeKHR::eUpdate : vk::BuildAccelerationStructureModeKHR::eBuild;
	build_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;
	build_info.srcAccelerationStructure = nullptr;

	// Find sizes
	vk::AccelerationStructureBuildSizesInfoKHR size_info{};
	uint32_t count = 1;
	size_info = device->getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build_info, count);

	if (!update) {
		Buffer *temp2 = new Buffer(*device, physical_device, size_info.accelerationStructureSize, { vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress }, { vk::MemoryPropertyFlagBits::eDeviceLocal });
		rtx_tlas_buffer = std::unique_ptr<Buffer>(temp2);
		debug_set_object_name(*device, (uint64_t)(VkBuffer)rtx_tlas_buffer.get(), vk::DebugReportObjectTypeEXT::eBuffer, "RTX TLAS BUFFER");

		vk::AccelerationStructureCreateInfoKHR create_info{};
		create_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;
		create_info.size = size_info.accelerationStructureSize;
		create_info.buffer = rtx_tlas_buffer->buffer;
		rtx_tlas = device->createAccelerationStructureKHRUnique(create_info);
		debug_set_object_name(*device, (uint64_t)(VkAccelerationStructureKHR)rtx_tlas.get(), vk::DebugReportObjectTypeEXT::eAccelerationStructureKHR, "RTX TLAS");
	}

	Buffer scratch(*device, physical_device, size_info.buildScratchSize, { vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress }, { vk::MemoryPropertyFlagBits::eDeviceLocal });
	build_info.srcAccelerationStructure = update ? *rtx_tlas : nullptr;
	build_info.dstAccelerationStructure = *rtx_tlas;
	build_info.scratchData.deviceAddress = scratch.address;

	vk::AccelerationStructureBuildRangeInfoKHR build_range(1, 0, 0, 0);

	vk::UniqueCommandBuffer cmd_buf = std::move(device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(*graphics_command_pool, vk::CommandBufferLevel::ePrimary, 1)).front());
	cmd_buf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	vk::MemoryBarrier barrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eAccelerationStructureWriteKHR);
	cmd_buf->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {}, barrier, nullptr, nullptr);
	cmd_buf->buildAccelerationStructuresKHR(build_info, &build_range);
	cmd_buf->end();
	graphics_queue.submit(vk::SubmitInfo(nullptr, nullptr, *cmd_buf, nullptr), nullptr);
	graphics_queue.waitIdle();

	rtx_tlas_address = device->getAccelerationStructureAddressKHR(vk::AccelerationStructureDeviceAddressInfoKHR(*rtx_tlas));
}
void VulkanApplication::build_rtx_pipeline() {
	// Allocate render image
	Image *img = new Image(
			*device, physical_device,
			swapchain->get_extent().width, swapchain->get_extent().height,
			vk::Format::eB8G8R8A8Unorm, vk::ImageTiling::eOptimal,
			{ vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage },
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::ImageAspectFlagBits::eColor);

	rtx_result_image = std::unique_ptr<Image>(img);

	// RTX_Layout
	std::vector<vk::DescriptorSetLayoutBinding> bindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eAccelerationStructureKHR, 1, { vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR }, nullptr),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, { vk::ShaderStageFlagBits::eRaygenKHR }, nullptr),
	};
	auto rtx_set_layout = device->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({}, bindings));
	vk::PushConstantRange push_constant{ vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR, 0, sizeof(RTPushConstant) };

	// Pool & set
	std::vector<vk::DescriptorPoolSize> pool_sizes{
		vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR, 1),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1)
	};
	rtx_pool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet }, 1, pool_sizes));
	rtx_set = std::move(device->allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(*rtx_pool, *rtx_set_layout)).front());

	// Write
	vk::WriteDescriptorSetAccelerationStructureKHR as_desc(*rtx_tlas);
	vk::WriteDescriptorSet as_descriptor_write{};
	as_descriptor_write.dstSet = *rtx_set;
	as_descriptor_write.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
	as_descriptor_write.pNext = &as_desc;
	as_descriptor_write.dstBinding = 0;
	as_descriptor_write.descriptorCount = 1;

	vk::DescriptorImageInfo image_info({}, rtx_result_image->image_view, vk::ImageLayout::eGeneral);
	vk::WriteDescriptorSet image_descriptor_write(*rtx_set, 1, 0, vk::DescriptorType::eStorageImage, image_info, nullptr, nullptr);

	std::vector<vk::WriteDescriptorSet> writes{ as_descriptor_write, image_descriptor_write };
	device->updateDescriptorSets(writes, nullptr);

	// Shaders
	auto rgen_code = read_file("resources/shaders/raytrace.rgen.spv");
	auto rmiss_code = read_file("resources/shaders/raytrace.rmiss.spv");
	auto rchit_code = read_file("resources/shaders/raytrace.rchit.spv");

	auto rgen_shader = device->createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, rgen_code.size(), reinterpret_cast<uint32_t *>(rgen_code.data())));
	auto rmiss_shader = device->createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, rmiss_code.size(), reinterpret_cast<uint32_t *>(rmiss_code.data())));
	auto rchit_shader = device->createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, rchit_code.size(), reinterpret_cast<uint32_t *>(rchit_code.data())));

	auto shader_stages = {
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eRaygenKHR, *rgen_shader, "main"),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eMissKHR, *rmiss_shader, "main"),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eClosestHitKHR, *rchit_shader, "main"),
	};

	shader_groups = {
		vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral, 0, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_NULL_HANDLE),
		vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral, 1, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_NULL_HANDLE),
		vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup, VK_SHADER_UNUSED_KHR, 2, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_NULL_HANDLE),
	};

	std::vector<vk::DescriptorSetLayout> layouts{
		*rtx_set_layout,
		swapchain->get_scene_descriptor_set(),
	};
	rtx_pipeline_layout = device->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, layouts, push_constant));
	vk::RayTracingPipelineCreateInfoKHR create_info(
			{},
			shader_stages, shader_groups, 1,
			nullptr, nullptr, nullptr,
			*rtx_pipeline_layout, nullptr, 0);
	rtx_pipeline = device->createRayTracingPipelineKHRUnique({}, {}, create_info).value;
}
void VulkanApplication::create_rtx_SBT() {
	const uint32_t group_size_aligned = (rtx_properties.shaderGroupHandleSize + rtx_properties.shaderGroupBaseAlignment - 1) & ~(rtx_properties.shaderGroupBaseAlignment - 1);
	const uint32_t shader_binding_table_size = group_size_aligned * shader_groups.size();

	auto buf = new Buffer(*device, physical_device, shader_binding_table_size, { vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress }, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible });
	shader_binding_table = std::unique_ptr<Buffer>(buf);

	std::vector<uint8_t> shader_handle_storage = device->getRayTracingShaderGroupHandlesKHR<uint8_t>(*rtx_pipeline, 0, shader_groups.size(), shader_binding_table_size);

	void *mapped = device->mapMemory(shader_binding_table->memory, 0, shader_binding_table_size);
	auto *data = reinterpret_cast<uint8_t *>(mapped);
	for (uint32_t i = 0; i < shader_groups.size(); i++) {
		memcpy(data, shader_handle_storage.data() + i * rtx_properties.shaderGroupHandleSize, rtx_properties.shaderGroupHandleSize);
		data += group_size_aligned;
	}
	device->unmapMemory(shader_binding_table->memory);
}
void VulkanApplication::raytrace(VulkanFrame &frame, int image_id) {
	frame.command_buffer->begin(vk::CommandBufferBeginInfo({}, nullptr));

	vk::ImageMemoryBarrier image_barrier({}, vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, rtx_result_image->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
	frame.command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, image_barrier);

	rtx_push_constants.clear_color = glm::vec4(0.1f, 0.1f, 0.1f, 1.f);
	rtx_push_constants.light_pos = glm::vec3(0.f, 0.f, 1.f);
	rtx_push_constants.light_intensity = 0.5f;
	rtx_push_constants.light_type = 0;

	frame.command_buffer->bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *rtx_pipeline);
	frame.command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *rtx_pipeline_layout, 0, { *rtx_set, frame.scene_descriptor_set.get() }, nullptr);
	frame.command_buffer->pushConstants(
			*rtx_pipeline_layout,
			{ vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR },
			0, sizeof(RTPushConstant), &rtx_push_constants);

	const uint32_t group_size = (rtx_properties.shaderGroupHandleSize + rtx_properties.shaderGroupBaseAlignment - 1) & ~(rtx_properties.shaderGroupBaseAlignment - 1);
	std::array<vk::StridedDeviceAddressRegionKHR, 4> strides{
		vk::StridedDeviceAddressRegionKHR(shader_binding_table->address + 0u * group_size, group_size, group_size),
		vk::StridedDeviceAddressRegionKHR(shader_binding_table->address + 1u * group_size, group_size, group_size),
		vk::StridedDeviceAddressRegionKHR(shader_binding_table->address + 2u * group_size, group_size, group_size),
		vk::StridedDeviceAddressRegionKHR(0u, 0u, 0u),
	};
	frame.command_buffer->traceRaysKHR(strides[0], strides[1], strides[2], strides[3], swapchain->get_extent().width, swapchain->get_extent().height, 1);

	image_barrier.image = swapchain->get_image(image_id);
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
	copy_region.extent = vk::Extent3D(swapchain->get_extent(), 1);
	frame.command_buffer->copyImage(rtx_result_image->image, vk::ImageLayout::eTransferSrcOptimal, swapchain->get_image(image_id), vk::ImageLayout::eTransferDstOptimal, copy_region);

	image_barrier.image = swapchain->get_image(image_id);
	image_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	image_barrier.dstAccessMask = vk::AccessFlags();
	image_barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	image_barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
	frame.command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, image_barrier);

	frame.command_buffer->end();
}
