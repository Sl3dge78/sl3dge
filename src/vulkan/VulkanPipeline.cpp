#include "VulkanPipeline.h"

#include "vulkan/Vertex.h"
#include "vulkan/VulkanHelper.h"

void VulkanPipeline::add_to_pool(const vk::DescriptorType type, const uint32_t count) {
	bool found = false;
	for (auto &s : pool_sizes) {
		if (s.type == type) {
			s.descriptorCount += count;
			found = true;
			break;
		}
	}

	if (!found) {
		pool_sizes.emplace_back(type, count);
	}
}
VulkanPipeline::~VulkanPipeline() {
	set.reset();
	pool.reset();
	pipeline.reset();
}
void VulkanPipeline::add_shader(const std::string path, const vk::ShaderStageFlagBits stage) {
}
void VulkanPipeline::add_descriptor(const vk::DescriptorType type, const vk::ArrayProxyNoTemporaries<const vk::DescriptorBufferInfo> &buffer_info, const uint32_t binding, vk::ShaderStageFlags stages, const uint32_t pool_amount, const uint32_t desc_amount) {
	// TODO : Handle arrays
	switch (type) {
		case vk::DescriptorType::eStorageBuffer:
		case vk::DescriptorType::eUniformBuffer:
			break;
		default:
			throw std::runtime_error("Called add_descriptor with buffer info, but type specified isn't a buffer");
			break;
	}

	add_to_pool(type, pool_amount);

	bindings.emplace_back(binding, type, desc_amount, stages, nullptr);

	vk::WriteDescriptorSet write(nullptr, binding, 0, type, nullptr, buffer_info, nullptr);
	descriptor_writes.push_back(write);
}
void VulkanPipeline::add_descriptor(const vk::DescriptorType type, const vk::ArrayProxyNoTemporaries<const vk::DescriptorImageInfo> &image_info, const uint32_t binding, vk::ShaderStageFlags stages, const uint32_t pool_amount, const uint32_t desc_amount) {
	switch (type) {
		case vk::DescriptorType::eCombinedImageSampler:
		case vk::DescriptorType::eStorageImage:
			break;
		default:
			throw std::runtime_error("Called add_descriptor with image info, but type specified isn't supported");
			break;
	}

	bindings.emplace_back(binding, type, desc_amount, stages, nullptr);

	add_to_pool(type, pool_amount);

	descriptor_writes.emplace_back(nullptr, binding, 0, type, image_info, nullptr, nullptr);
}
void VulkanPipeline::add_descriptor(const vk::DescriptorSetLayoutBinding binding, const vk::DescriptorPoolSize pool_size, const vk::WriteDescriptorSet write) {
	bindings.push_back(binding);
	descriptor_writes.push_back(write);
}
void VulkanPipeline::add_push_constant(const vk::PushConstantRange range) {
	push_constants.push_back(range);
}
void VulkanPipeline::bind(vk::CommandBuffer cmd) {
	cmd.bindPipeline(bind_point, *pipeline);
}
void VulkanPipeline::bind_descriptors(vk::CommandBuffer cmd) {
	cmd.bindDescriptorSets(bind_point, *layout, 0, *set, nullptr);
}
void VulkanPipeline::bind_push_constants(vk::CommandBuffer cmd, const uint32_t id, void *value) {
	cmd.pushConstants(*layout, push_constants[id].stageFlags, push_constants[id].offset, push_constants[id].size, value);
}
void VulkanPipeline::build_descriptors(vk::Device device) {
	set_layout = device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({}, bindings));
	if (push_constants.size() > 0) {
		layout = device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, *set_layout, push_constants));
	} else {
		layout = device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo({}, *set_layout, nullptr));
	}

	pool = device.createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet }, 1, pool_sizes));

	set = std::move(device.allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(*pool, *set_layout)).front());

	for (auto &w : descriptor_writes) {
		w.setDstSet(*set);
	}
	device.updateDescriptorSets(descriptor_writes, nullptr);
}
void VulkanPipeline::create_cache(vk::Device device) {
	cache = device.createPipelineCacheUnique(vk::PipelineCacheCreateInfo());
}

void GraphicsPipeline::build_pipeline(vk::Device device, vk::Extent2D extent, vk::RenderPass render_pass) {
	bind_point = vk::PipelineBindPoint::eGraphics; // C'est degueu
	// Shaders
	auto v_code = read_file("resources/shaders/general.vert.spv");
	auto v_shader = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, v_code.size(), reinterpret_cast<uint32_t *>(v_code.data())));
	shader_stages.push_back(vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *v_shader, "main"));
	auto f_code = read_file("resources/shaders/general.frag.spv");
	auto f_shader = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, f_code.size(), reinterpret_cast<uint32_t *>(f_code.data())));
	shader_stages.push_back(vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *f_shader, "main"));
	graphics_create_info.setStages(shader_stages);

	create_cache(device);

	vk::PipelineVertexInputStateCreateInfo vertex_input{};
	auto desc = Vertex::get_attribute_descriptions();
	vertex_input.vertexAttributeDescriptionCount = desc.size();
	vertex_input.pVertexAttributeDescriptions = desc.data();
	auto bindings = Vertex::get_binding_description();
	vertex_input.vertexBindingDescriptionCount = bindings.size();
	vertex_input.pVertexBindingDescriptions = bindings.data();
	graphics_create_info.pVertexInputState = &vertex_input;

	vk::PipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.setTopology(vk::PrimitiveTopology::eTriangleList);
	input_assembly.primitiveRestartEnable = false;
	graphics_create_info.pInputAssemblyState = &input_assembly;

	vk::PipelineViewportStateCreateInfo viewport_state{};
	vk::Rect2D scissor({ 0, 0 }, extent);
	viewport_state.setScissors(scissor);
	vk::Viewport viewport(0.0f, 0.0f, extent.width, extent.height, 0.0f, 1.0f);
	viewport_state.setViewports(viewport);
	graphics_create_info.pViewportState = &viewport_state;

	vk::PipelineRasterizationStateCreateInfo rasterization{};
	rasterization.depthClampEnable = false;
	rasterization.rasterizerDiscardEnable = false;
	rasterization.polygonMode = vk::PolygonMode::eFill;
	rasterization.cullMode = vk::CullModeFlagBits::eNone;
	rasterization.frontFace = vk::FrontFace::eCounterClockwise;
	rasterization.depthBiasEnable = false;
	rasterization.lineWidth = 1.0f;
	graphics_create_info.pRasterizationState = &rasterization;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.sampleShadingEnable = false;
	multisampling.minSampleShading = 0.f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = false;
	multisampling.alphaToOneEnable = false;
	graphics_create_info.pMultisampleState = &multisampling;

	vk::PipelineDepthStencilStateCreateInfo depth{};
	depth.depthTestEnable = true;
	depth.depthWriteEnable = true;
	depth.depthCompareOp = vk::CompareOp::eLess;
	depth.depthBoundsTestEnable = false;
	depth.stencilTestEnable = false;
	graphics_create_info.pDepthStencilState = &depth;

	vk::PipelineColorBlendStateCreateInfo color_blend{};
	color_blend.logicOpEnable = false;
	color_blend.logicOp = vk::LogicOp::eCopy;
	vk::PipelineColorBlendAttachmentState blend_attachement{};
	blend_attachement.colorWriteMask = { vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
	blend_attachement.blendEnable = false;
	color_blend.attachmentCount = 1;
	color_blend.pAttachments = &blend_attachement;
	graphics_create_info.pColorBlendState = &color_blend;

	graphics_create_info.layout = get_layout();
	graphics_create_info.renderPass = render_pass;

	create_pipeline(device);
}
void GraphicsPipeline::create_pipeline(vk::Device device) {
	pipeline = device.createGraphicsPipelineUnique(get_cache(), graphics_create_info);
}

void RaytracingPipeline::build_pipeline(vk::Device device) {
	bind_point = vk::PipelineBindPoint::eRayTracingKHR; // C'est degueu
	// Shaders
	auto rgen_code = read_file("resources/shaders/raytrace.rgen.spv");
	auto rmiss_code = read_file("resources/shaders/raytrace.rmiss.spv");
	auto rmiss_shadow_code = read_file("resources/shaders/shadow.rmiss.spv");
	auto rchit_code = read_file("resources/shaders/raytrace.rchit.spv");

	auto rgen_shader = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, rgen_code.size(), reinterpret_cast<uint32_t *>(rgen_code.data())));
	auto rmiss_shader = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, rmiss_code.size(), reinterpret_cast<uint32_t *>(rmiss_code.data())));
	auto rmiss_shadow_shader = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, rmiss_shadow_code.size(), reinterpret_cast<uint32_t *>(rmiss_shadow_code.data())));
	auto rchit_shader = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, rchit_code.size(), reinterpret_cast<uint32_t *>(rchit_code.data())));

	debug_name_object(device, uint64_t(VkShaderModule(rgen_shader.get())), vk::ObjectType::eShaderModule, "raytrace.rgen");
	debug_name_object(device, uint64_t(VkShaderModule(rmiss_shader.get())), vk::ObjectType::eShaderModule, "raytrace.rmiss");
	debug_name_object(device, uint64_t(VkShaderModule(rmiss_shadow_shader.get())), vk::ObjectType::eShaderModule, "shadow.rmiss");
	debug_name_object(device, uint64_t(VkShaderModule(rchit_shader.get())), vk::ObjectType::eShaderModule, "raytrace.rchit");

	shader_stages.push_back(vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eRaygenKHR, *rgen_shader, "main"));
	shader_stages.push_back(vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eMissKHR, *rmiss_shader, "main"));
	shader_stages.push_back(vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eMissKHR, *rmiss_shadow_shader, "main"));
	shader_stages.push_back(vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eClosestHitKHR, *rchit_shader, "main"));

	create_cache(device);

	shader_groups = {
		vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral, 0, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_NULL_HANDLE),
		vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral, 1, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_NULL_HANDLE),
		vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eGeneral, 2, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_NULL_HANDLE),
		vk::RayTracingShaderGroupCreateInfoKHR(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup, VK_SHADER_UNUSED_KHR, 3, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_NULL_HANDLE),
	};

	create_info = vk::RayTracingPipelineCreateInfoKHR(
			{},
			shader_stages, shader_groups, 1,
			nullptr, nullptr, nullptr,
			this->get_layout(), nullptr, 0);

	create_pipeline(device);
}
void RaytracingPipeline::create_pipeline(vk::Device device) {
	pipeline = device.createRayTracingPipelineKHRUnique({}, get_cache(), create_info).value;
}
