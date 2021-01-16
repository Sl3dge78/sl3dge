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

	Input::cleanup();

	scene.reset();

	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	// SDL
	SDL_DestroyWindow(window);
	SDL_Log("Cleanup done! Bye bye!");
	IMG_Quit();
	SDL_Quit();
}
void VulkanApplication::run() {
	scene = std::make_unique<Scene>();
	load();
	SDL_Log("Resources loaded");
	scene->build_BLAS(*this, { vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction });
	SDL_Log("BLAS created");
	scene->build_TLAS(*this, { vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace });
	SDL_Log("TLAS created");
	scene->allocate_uniform_buffer(*this);
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

		// Don't draw if the app is minimized
		if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)) {
			draw_frame();
		}
		device->waitIdle();
	}
}
void VulkanApplication::rasterize_scene(VulkanFrame &frame) {
	// FRAG UBO
	/*
	frame.scene_buffer->write_data(&fubo, sizeof(fubo), sizeof(CameraMatrices));
	frame.command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, swapchain->get_pipeline());
	frame.command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, swapchain->get_pipeline_layout(), 0, frame.scene_descriptor_set.get(), nullptr);
	scene->rasterize(frame);
	 */
}
void VulkanApplication::draw_ui(VulkanFrame &frame) {
	// UI
	ImGui::Render();
	auto imgui_data = ImGui::GetDrawData();
	frame.begin_render_pass();
	ImGui_ImplVulkan_RenderDrawData(imgui_data, frame.command_buffer.get());
	frame.end_render_pass();
}
void VulkanApplication::draw_frame() {
	vk::Semaphore image_acquired_semaphore = swapchain->get_image_acquired_semaphore(semaphore_index);
	vk::Semaphore render_complete_semaphore = swapchain->get_render_complete_semaphore(semaphore_index);

	// Acquire next image
	uint32_t image_id;
	vk::Result result;
	try {
		// Get next image and trigger image_acquired_semaphore once ready
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

	// Wait for the frame to be finished
	auto frame = swapchain->get_frame(image_id);
	if (device->waitForFences(frame->fence.get(), true, UINT64_MAX) != vk::Result::eSuccess) {
		throw std::runtime_error("Waiting for frame timed out!");
	}
	device->resetFences(frame->fence.get());

	frame->command_buffer->begin(vk::CommandBufferBeginInfo({}, nullptr));
	raytrace(*frame, image_id);
	draw_ui(*frame);
	frame->command_buffer->end();

	// Submit Queue
	vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	vk::SubmitInfo si(image_acquired_semaphore, flags, frame->command_buffer.get(), render_complete_semaphore);
	graphics_queue.submit(si, frame->fence.get());

	// Present
	vk::SwapchainKHR swap = swapchain->get_swapchain();
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
	// TODO : do this more cleanly
	auto supported_features = physical_device.getFeatures2<
			vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceBufferDeviceAddressFeatures,
			vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
			vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
			vk::PhysicalDeviceDescriptorIndexingFeatures>();
	vk::DeviceCreateInfo ci({}, req_queues, req_validation_layers, req_device_extensions, nullptr);
	ci.setPNext(&supported_features.get<vk::PhysicalDeviceFeatures2>());
	device = physical_device.createDeviceUnique(ci);

	VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);

	graphics_queue = device->getQueue(queue_family_indices.graphics_family.value(), 0);
	present_queue = device->getQueue(queue_family_indices.present_family.value(), 0);
	transfer_queue = device->getQueue(queue_family_indices.transfer_family.value(), 0);
}
void VulkanApplication::create_texture_image() {
	/*
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
	*/
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

int VulkanApplication::get_graphics_family_index() const {
	return queue_family_indices.graphics_family.value();
}
int VulkanApplication::get_transfer_family_index() const {
	return queue_family_indices.transfer_family.value();
}

void VulkanApplication::init_rtx() {
	auto properties = physical_device.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
	rtx_properties = properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
	if (rtx_properties.maxRayRecursionDepth <= 1) {
		SDL_LogError(0, "Device doesn't support ray recursion!");
		throw std::runtime_error("Device doesn't support ray recursion!");
	}
}
void VulkanApplication::build_rtx_pipeline() {
	// Allocate render image TODO : recreate on swapchain recreation, link to swapchain??
	/*
	VkFormatProperties properties;
	vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_B8G8R8A8_SRGB, &properties);
	if (properties.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) {
		SDL_Log("Ok storage");
	}
	if (properties.linearTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT) {
		SDL_Log("Ok transfer");
	}
	*/
	Image *img = new Image(
			*device, physical_device,
			swapchain->get_extent().width, swapchain->get_extent().height,
			swapchain->get_format(), vk::ImageTiling::eOptimal,
			{ vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage },
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::ImageAspectFlagBits::eColor);

	rtx_result_image = std::unique_ptr<Image>(img);

	// TODO : move this shit in Scene?
	uint32_t mesh_count = scene->meshes.size();
	uint32_t material_count = scene->materials.size();

	{ // Descriptor Layout
		std::vector<vk::DescriptorSetLayoutBinding> bindings{
			// Acceleration structure
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eAccelerationStructureKHR, 1, { vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR }, nullptr),
			// Final image
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, { vk::ShaderStageFlagBits::eRaygenKHR }, nullptr),
			// Camera matrices
			vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eRaygenKHR, nullptr),
			// Scene description
			vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, { vk::ShaderStageFlagBits::eClosestHitKHR }, nullptr),
			// vtx buffer
			vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, mesh_count, { vk::ShaderStageFlagBits::eClosestHitKHR }, nullptr),
			// idx buffer
			vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, mesh_count, { vk::ShaderStageFlagBits::eClosestHitKHR }, nullptr),
			// Texture buffer
			vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eCombinedImageSampler, 1, { vk::ShaderStageFlagBits::eClosestHitKHR }, nullptr),
		};

		vk::DescriptorSetLayoutCreateInfo set_layout_ci({}, bindings);
		rtx_set_layout = device->createDescriptorSetLayoutUnique(set_layout_ci);

		// Constants
		vk::PushConstantRange push_constant{ vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR, 0, sizeof(RTPushConstant) };

		std::vector<vk::DescriptorSetLayout> layouts{
			*rtx_set_layout,
		};
		rtx_layout = device->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, layouts, push_constant));
	}

	{ // Create Descriptor pool

		std::vector<vk::DescriptorPoolSize> pool_sizes{
			vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR, 1),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 3),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1),
		};
		rtx_pool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet }, 1, pool_sizes));
	}

	// Descriptor set
	rtx_set = std::move(device->allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(*rtx_pool, *rtx_set_layout)).front());

	{ // Descriptor writes
		std::vector<vk::WriteDescriptorSet> writes;
		// Acceleration structure = binding 0
		std::vector<vk::AccelerationStructureKHR> as{ scene->get_tlas() };
		vk::WriteDescriptorSetAccelerationStructureKHR as_desc(as);
		vk::WriteDescriptorSet as_descriptor_write(*rtx_set, 0, 0, 1, vk::DescriptorType::eAccelerationStructureKHR, nullptr, nullptr, nullptr);
		as_descriptor_write.pNext = &as_desc;
		writes.push_back(as_descriptor_write);

		// Image = binding 1
		vk::DescriptorImageInfo image_info({}, rtx_result_image->image_view, vk::ImageLayout::eGeneral);
		vk::WriteDescriptorSet image_descriptor_write(*rtx_set, 1, 0, vk::DescriptorType::eStorageImage, image_info, nullptr, nullptr);
		writes.push_back(image_descriptor_write);

		// Camera matrices = binding 2
		vk::DescriptorBufferInfo bi_camera_matrices(scene->camera.buffer->buffer, 0, VK_WHOLE_SIZE);
		vk::WriteDescriptorSet camera_write(*rtx_set, 2, 0, vk::DescriptorType::eUniformBuffer, nullptr, bi_camera_matrices, nullptr);
		writes.push_back(camera_write);

		// Scene = binding 3
		vk::DescriptorBufferInfo bi_scene(scene->scene_desc_buffer->buffer, 0, VK_WHOLE_SIZE);
		vk::WriteDescriptorSet scene_write(*rtx_set, 3, 0, vk::DescriptorType::eStorageBuffer, nullptr, bi_scene, nullptr);
		writes.push_back(scene_write);

		// Storage buffers
		// Vertex buffers = binding 4
		// index buffers = binding 5
		std::vector<vk::DescriptorBufferInfo> bi_vtx;
		std::vector<vk::DescriptorBufferInfo> bi_idx;
		for (auto &mesh : scene->meshes) {
			bi_vtx.emplace_back(mesh->vertex_buffer->buffer, 0, VK_WHOLE_SIZE);
			bi_idx.emplace_back(mesh->index_buffer->buffer, 0, VK_WHOLE_SIZE);
		}
		vk::WriteDescriptorSet vtx_writes(*rtx_set, 4, 0, mesh_count, vk::DescriptorType::eStorageBuffer, nullptr, bi_vtx.data(), nullptr);
		vk::WriteDescriptorSet idx_writes(*rtx_set, 5, 0, mesh_count, vk::DescriptorType::eStorageBuffer, nullptr, bi_idx.data(), nullptr);
		writes.push_back(vtx_writes);
		writes.push_back(idx_writes);

		// Texture sampler = binding 6
		std::vector<vk::DescriptorImageInfo> images_info;
		for (auto &mat : scene->materials) {
			images_info.emplace_back(*texture_sampler, mat->texture->image_view, vk::ImageLayout::eShaderReadOnlyOptimal);
		}
		vk::WriteDescriptorSet texture_write(*rtx_set, 6, 0, material_count, vk::DescriptorType::eCombinedImageSampler, images_info.data(), nullptr, nullptr);
		writes.push_back(texture_write);

		device->updateDescriptorSets(writes, nullptr);
	}

	// Shaders
	auto rgen_code = read_file("resources/shaders/raytrace.rgen.spv");
	auto rmiss_code = read_file("resources/shaders/raytrace.rmiss.spv");
	auto rmiss_shadow_code = read_file("resources/shaders/shadow.rmiss.spv");
	auto rchit_code = read_file("resources/shaders/raytrace.rchit.spv");

	auto rgen_shader = device->createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, rgen_code.size(), reinterpret_cast<uint32_t *>(rgen_code.data())));
	auto rmiss_shader = device->createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, rmiss_code.size(), reinterpret_cast<uint32_t *>(rmiss_code.data())));
	auto rmiss_shadow_shader = device->createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, rmiss_shadow_code.size(), reinterpret_cast<uint32_t *>(rmiss_shadow_code.data())));
	auto rchit_shader = device->createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, rchit_code.size(), reinterpret_cast<uint32_t *>(rchit_code.data())));

	auto shader_stages = {
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eRaygenKHR, *rgen_shader, "main"),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eMissKHR, *rmiss_shader, "main"),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eMissKHR, *rmiss_shadow_shader, "main"),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eClosestHitKHR, *rchit_shader, "main"),
	};

	shader_groups = {
		vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral, 0, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_NULL_HANDLE),
		vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral, 1, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_NULL_HANDLE),
		vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral, 2, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_NULL_HANDLE),
		vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup, VK_SHADER_UNUSED_KHR, 3, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_NULL_HANDLE),
	};

	vk::RayTracingPipelineCreateInfoKHR create_info(
			{},
			shader_stages, shader_groups, 1,
			nullptr, nullptr, nullptr,
			*rtx_layout, nullptr, 0);
	rtx_pipeline = device->createRayTracingPipelineKHRUnique({}, {}, create_info).value;
}
void VulkanApplication::create_rtx_SBT() {
	const uint32_t group_size_aligned = (rtx_properties.shaderGroupHandleSize + rtx_properties.shaderGroupBaseAlignment - 1) & ~(rtx_properties.shaderGroupBaseAlignment - 1);
	const uint32_t shader_binding_table_size = group_size_aligned * shader_groups.size();

	shader_binding_table = std::unique_ptr<Buffer>(new Buffer(*device, physical_device, shader_binding_table_size, { vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress }, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible }));

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
	vk::ImageMemoryBarrier image_barrier({}, vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, rtx_result_image->image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
	frame.command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, image_barrier);

	frame.command_buffer->bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *rtx_pipeline);
	frame.command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, *rtx_layout, 0, *rtx_set, nullptr);
	frame.command_buffer->pushConstants(
			*rtx_layout,
			{ vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR },
			0, sizeof(RTPushConstant), &rtx_push_constants);

	const uint32_t group_size = (rtx_properties.shaderGroupHandleSize + rtx_properties.shaderGroupBaseAlignment - 1) & ~(rtx_properties.shaderGroupBaseAlignment - 1);
	std::vector<vk::StridedDeviceAddressRegionKHR> strides{
		vk::StridedDeviceAddressRegionKHR(shader_binding_table->address + 0u * group_size, group_size, group_size), // Rgen
		vk::StridedDeviceAddressRegionKHR(shader_binding_table->address + 1u * group_size, group_size, group_size), // Miss
		vk::StridedDeviceAddressRegionKHR(shader_binding_table->address + 3u * group_size, group_size, group_size), // Hit
		vk::StridedDeviceAddressRegionKHR(0u, 0u, 0u),
	};
	frame.command_buffer->traceRaysKHR(strides[0], strides[1], strides[2], strides[3], swapchain->get_extent().width, swapchain->get_extent().height, 1);

	image_barrier.image = swapchain->get_image(image_id);
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
	copy_region.extent = vk::Extent3D(swapchain->get_extent(), 1);
	frame.command_buffer->copyImage(rtx_result_image->image, vk::ImageLayout::eTransferSrcOptimal, swapchain->get_image(image_id), vk::ImageLayout::eTransferDstOptimal, copy_region);

	image_barrier.image = swapchain->get_image(image_id);
	image_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	image_barrier.dstAccessMask = vk::AccessFlagBits::eIndexRead;
	image_barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	image_barrier.newLayout = vk::ImageLayout::eGeneral;
	frame.command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eVertexInput, {}, nullptr, nullptr, image_barrier);
}
