//
// Created by Sledge on 2021-01-02.
//

#include "Swapchain.h"

SwapchainCreateInfo Swapchain::info;

Swapchain::Swapchain() {
	info.device.waitIdle();

	create_swapchain(info.device, info.physical_device, info.surface, info.window);
	if (log)
		SDL_Log("Swapchain created");
	create_render_pass(info.device, info.physical_device);
	if (log)
		SDL_Log("Render pass created");
	create_descriptors(info.device);
	if (log)
		SDL_Log("Descriptors created");
	create_depth_resources(info.device, info.physical_device);
	if (log)
		SDL_Log("Depth resources created");
	create_graphics_pipeline(info.device);
	if (log)
		SDL_Log("Pipeline created");
	create_frames(info.device, info.physical_device, info.command_pool, info.texture_sampler, info.texture_image_view);
	if (log)
		SDL_Log("Frames created");
	create_semaphores(info.device);
	if (log)
		SDL_Log("Semaphores created");
	create_imgui_context(info);
	if (log)
		SDL_Log("Imgui created");

	SDL_Log("Swapchain created");
}
Swapchain::~Swapchain() {
	ImGui_ImplVulkan_Shutdown();
}
void Swapchain::create_swapchain(vk::Device device, vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, SDL_Window *window) {
	auto capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
	auto surface_fmt = choose_surface_format(physical_device, surface);
	auto present_mode = choose_present_mode(physical_device, surface);
	extent = choose_extent(capabilities, window);
	uint32_t req_image_count = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && req_image_count > capabilities.maxImageCount) {
		req_image_count = capabilities.maxImageCount;
	}
	auto family_indices = find_queue_families(physical_device, surface);
	std::array<const uint32_t, 2> queue_indices({ family_indices.graphics_family.value(), family_indices.present_family.value() });

	vk::SwapchainCreateInfoKHR ci({},
			surface,
			req_image_count,
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
	swapchain = device.createSwapchainKHRUnique(ci);
	format = surface_fmt.format;
	images = device.getSwapchainImagesKHR(*swapchain);
	image_count = images.size();
}
void Swapchain::create_render_pass(vk::Device device, vk::PhysicalDevice physical_device) {
	std::vector<vk::AttachmentDescription> attachments;
	vk::AttachmentDescription color_attachment(
			{},
			format,
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
			find_depth_format(physical_device),
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

	render_pass = device.createRenderPassUnique(vk::RenderPassCreateInfo({}, attachments, subpass, dependency));
}
void Swapchain::create_descriptors(vk::Device device) {
	vk::DescriptorSetLayoutBinding ubo(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutBinding sampler(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr);
	vk::DescriptorSetLayoutBinding frag(2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment, nullptr);
	std::array<vk::DescriptorSetLayoutBinding, 3> bindings = { ubo, sampler, frag };
	descriptor_set_layout = device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({}, bindings));

	vk::DescriptorPoolSize a(vk::DescriptorType::eUniformBuffer, image_count);
	vk::DescriptorPoolSize b(vk::DescriptorType::eCombinedImageSampler, image_count);
	std::array<vk::DescriptorPoolSize, 2> pool_sizes = { a, b };
	descriptor_pool = device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet }, image_count, pool_sizes));
}
void Swapchain::create_depth_resources(vk::Device device, vk::PhysicalDevice physical_device) {
	vk::Format depth_format = find_depth_format(physical_device);
	depth_image = std::unique_ptr<Image>(new Image(device, physical_device, extent.width, extent.height, depth_format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth));
}
void Swapchain::create_graphics_pipeline(vk::Device device) {
	auto vertex_shader_code = read_file("resources/shaders/triangle.vert.spv");
	auto fragment_shader_code = read_file("resources/shaders/triangle.frag.spv");

	auto vtx_shader = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, vertex_shader_code.size(), reinterpret_cast<uint32_t *>(vertex_shader_code.data())));
	auto frag_shader = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, fragment_shader_code.size(), reinterpret_cast<uint32_t *>(fragment_shader_code.data())));

	auto stages = {
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *vtx_shader, "main", nullptr),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *frag_shader, "main", nullptr)
	};
	auto a = Vertex::get_binding_description();
	auto b = Vertex::get_attribute_descriptions();
	vk::PipelineVertexInputStateCreateInfo vertex_input({}, a, b);

	vk::PipelineInputAssemblyStateCreateInfo input_ci({}, vk::PrimitiveTopology::eTriangleList, false);

	vk::Viewport viewport(0, 0, extent.width, extent.height, 0, 1);
	vk::Rect2D scissor({ 0, 0 }, extent);
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

	pipeline_layout = device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, *descriptor_set_layout));

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
			*pipeline_layout,
			*render_pass,
			0,
			nullptr,
			-1);

	graphics_pipeline = device.createGraphicsPipelineUnique(nullptr, pipeline_ci).value;
}
void Swapchain::create_frames(vk::Device device, vk::PhysicalDevice physical_device, vk::CommandPool command_pool, vk::Sampler texture_sampler, vk::ImageView texture_image_view) {
	frames.resize(image_count);
	for (uint32_t i = 0; i < images.size(); i++) {
		frames[i].init_frame(device);
		frames[i].image_view = create_image_view(device, images[i], format, vk::ImageAspectFlagBits::eColor);
		frames[i].create_framebuffer(extent, *render_pass, depth_image->image_view);
		frames[i].create_command_buffers(command_pool);
		frames[i].create_sync_objects();
		frames[i].create_uniform_buffer(physical_device);
		frames[i].create_descriptor_set(*descriptor_pool, *descriptor_set_layout, texture_sampler, texture_image_view);
	}
}
void Swapchain::create_semaphores(vk::Device device) {
	frame_semaphores.resize(image_count);

	for (auto &fs : frame_semaphores) {
		VkSemaphoreCreateInfo ci{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};
		fs.image_aquired = device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
		fs.render_complete = device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
	}
}
void Swapchain::create_imgui_context(SwapchainCreateInfo info) {
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

	imgui_descriptor_pool = info.device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet }, 1000 * uint32_t(pool_sizes.size()), pool_sizes));

	ImGui_ImplVulkan_InitInfo imgui_init_info{};
	imgui_init_info.Instance = info.instance;
	imgui_init_info.PhysicalDevice = info.physical_device;
	imgui_init_info.Device = info.device;
	imgui_init_info.QueueFamily = info.queue_family;
	imgui_init_info.Queue = info.queue;
	imgui_init_info.PipelineCache = VK_NULL_HANDLE;
	imgui_init_info.DescriptorPool = *imgui_descriptor_pool;
	imgui_init_info.Allocator = nullptr;
	imgui_init_info.MinImageCount = 2;
	imgui_init_info.ImageCount = uint32_t(frames.size());
	imgui_init_info.CheckVkResultFn = nullptr;
	imgui_init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	ImGui_ImplVulkan_Init(&imgui_init_info, *render_pass);

	frames[0].command_buffer->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr));
	ImGui_ImplVulkan_CreateFontsTexture(frames[0].command_buffer.get());
	frames[0].command_buffer->end();

	vk::SubmitInfo submit(nullptr, nullptr, frames[0].command_buffer.get(), nullptr);
	info.queue.submit(submit, nullptr);
	info.queue.waitIdle();
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}
vk::SurfaceFormatKHR Swapchain::choose_surface_format(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
	for (const auto &fmt : physical_device.getSurfaceFormatsKHR(surface)) {
		if (fmt.format == vk::Format::eB8G8R8A8Srgb && fmt.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
			return fmt;
		}
	}
	return physical_device.getSurfaceFormatsKHR(surface).front();
}
vk::PresentModeKHR Swapchain::choose_present_mode(const vk::PhysicalDevice physical_device, const vk::SurfaceKHR surface) {
	for (const auto &mode : physical_device.getSurfacePresentModesKHR(surface)) {
		if (mode == vk::PresentModeKHR::eMailbox) {
			return mode;
		}
	}
	SDL_Log("Present mode: VK_PRESENT_MODE_FIFO_KHR");
	return vk::PresentModeKHR::eFifo;
}
vk::Extent2D Swapchain::choose_extent(const vk::SurfaceCapabilitiesKHR &capabilities, SDL_Window *window) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		int w, h;
		SDL_Vulkan_GetDrawableSize(window, &w, &h);
		vk::Extent2D ext = { uint32_t(w), uint32_t(h) };
		ext.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
		ext.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));
		return ext;
	}
}
vk::Format Swapchain::find_depth_format(vk::PhysicalDevice physical_device) {
	return find_supported_format(physical_device,
			{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}