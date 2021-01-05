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
		mesh.load(this);
	}
	SDL_Log("Resources loaded");

	build_BLAS({ vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction });

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
	frame.uniform_buffer->write_data(&vubo, sizeof(vubo));
	// FRAG UBO
	frame.uniform_buffer->write_data(&fubo, sizeof(fubo), sizeof(MeshUBO) + sizeof(vubo));

	frame.command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, swapchain->get_pipeline());
	frame.command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, swapchain->get_pipeline_layout(), 0, frame.descriptor_set.get(), nullptr);

	for (auto &mesh : meshes) {
		mesh.draw(frame);
	}

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
	frame->begin_render_pass();
	draw_scene(*frame); // Render 3D objects in the scene
	frame->end_render_pass();

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
	}

	SDL_Log("Required extensions:");
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
	uint32_t nb_blas = meshes.size();
	std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> build_infos;
	std::vector<vk::UniqueAccelerationStructureKHR> acceleration_structures;
	vk::DeviceSize max_scratch;

	std::vector<vk::DeviceSize> original_sizes;
	std::vector<std::unique_ptr<Buffer>> buffers;

	for (auto &mesh : meshes) {
		vk::AccelerationStructureBuildGeometryInfoKHR build_info{};
		build_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
		build_info.setFlags(flags);
		build_info.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
		build_info.setGeometries(mesh.geometry);

		uint32_t max_primitive_count = mesh.range_info.primitiveCount;
		auto size_info = device->getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build_info, max_primitive_count);
		vk::AccelerationStructureCreateInfoKHR create_info({}, {}, {}, size_info.accelerationStructureSize, vk::AccelerationStructureTypeKHR::eBottomLevel, {});

		Buffer *buf = new Buffer(*device, physical_device, create_info.size, { vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress }, vk::MemoryPropertyFlagBits::eDeviceLocal);
		create_info.buffer = buf->buffer;
		auto as = device->createAccelerationStructureKHRUnique(create_info); // TODO : wrapper class

		build_info.setDstAccelerationStructure(*as);
		max_scratch = std::max(max_scratch, size_info.buildScratchSize);

		acceleration_structures.emplace_back(std::move(as));
		build_infos.emplace_back(build_info);
		original_sizes.emplace_back(size_info.accelerationStructureSize);
		buffers.emplace_back(std::unique_ptr<Buffer>(buf));
	}

	Buffer scratch_buffer(*device, physical_device, max_scratch, { vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer }, vk::MemoryPropertyFlagBits::eDeviceLocal);

	bool do_compaction = (flags & vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction) == vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction;
	auto query_pool = device->createQueryPoolUnique(vk::QueryPoolCreateInfo({}, vk::QueryType::eAccelerationStructureCompactedSizeKHR, nb_blas, {}));
	{
		std::vector<vk::CommandBuffer> cmd_buffs = device->allocateCommandBuffers(vk::CommandBufferAllocateInfo(*graphics_command_pool, vk::CommandBufferLevel::ePrimary, nb_blas));
		for (int i = 0; i < nb_blas; i++) {
			build_infos[i].scratchData.deviceAddress = scratch_buffer.address;

			cmd_buffs[i].begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
			cmd_buffs[i].buildAccelerationStructuresKHR(build_infos[i], &meshes[i].range_info);

			vk::MemoryBarrier barrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR, vk::AccessFlagBits::eAccelerationStructureReadKHR);
			cmd_buffs[i].pipelineBarrier(
					vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
					vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
					{}, barrier, nullptr, nullptr);

			if (do_compaction) {
				cmd_buffs[i].writeAccelerationStructuresPropertiesKHR(acceleration_structures[i].get(), vk::QueryType::eAccelerationStructureCompactedSizeKHR, *query_pool, i);
			}

			cmd_buffs[i].end();
		}
		graphics_queue.submit(vk::SubmitInfo(nullptr, {}, cmd_buffs, nullptr));
		graphics_queue.waitIdle();
		device->freeCommandBuffers(*graphics_command_pool, cmd_buffs);
	}
	if (do_compaction) {
		vk::UniqueCommandBuffer cmd_buf = std::move(device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(*graphics_command_pool, vk::CommandBufferLevel::ePrimary, 1)).front());
		cmd_buf->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr));

		std::vector<vk::DeviceSize> compact_sizes = device->getQueryPoolResults<vk::DeviceSize>(*query_pool, 0, nb_blas, nb_blas * sizeof(vk::Device), sizeof(vk::Device), vk::QueryResultFlagBits::eWait).value;
		std::vector<std::unique_ptr<Buffer>> compact_buffers;

		int stat_total_ori_size = 0;
		int stat_totat_compact_size = 0;
		std::vector<vk::UniqueAccelerationStructureKHR> old_acceleration_structures(nb_blas);
		for (int i = 0; i < nb_blas; i++) {
			stat_total_ori_size += original_sizes[i];
			stat_totat_compact_size += compact_sizes[i];

			Buffer *buf = new Buffer(*device, physical_device, compact_sizes[i], { vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress }, vk::MemoryPropertyFlagBits::eDeviceLocal);
			vk::AccelerationStructureCreateInfoKHR as_ci({}, buf->buffer, {}, compact_sizes[i], vk::AccelerationStructureTypeKHR::eBottomLevel, {});
			auto as = device->createAccelerationStructureKHRUnique(as_ci);

			cmd_buf->copyAccelerationStructureKHR(vk::CopyAccelerationStructureInfoKHR(acceleration_structures[i].get(), *as, vk::CopyAccelerationStructureModeKHR::eCompact));
			old_acceleration_structures[i] = std::move(acceleration_structures[i]);
			acceleration_structures[i] = std::move(as);
			compact_buffers.emplace_back(std::unique_ptr<Buffer>(buf));
		}
		cmd_buf->end();
		graphics_queue.submit(vk::SubmitInfo(nullptr, {}, *cmd_buf, nullptr));
		graphics_queue.waitIdle();
		SDL_Log("RTX Blas: reducing from %u to %u = %u (%2.2f%s smaller) ",
				stat_total_ori_size,
				stat_totat_compact_size,
				stat_total_ori_size - stat_totat_compact_size,
				(stat_total_ori_size - stat_totat_compact_size) / float(stat_total_ori_size) * 100.f, "%");
	}
}
