#include "VulkanPipeline.h"

#include "scene/Vertex.h"
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
void VulkanPipeline::add_shader(const std::string path, const vk::ShaderStageFlagBits stage) {
}
void VulkanPipeline::create_render_pass(std::vector<vk::AttachmentDescription> attachments, vk::SubpassDescription subpass, vk::SubpassDependency dependency) {
	render_pass = device.createRenderPassUnique(vk::RenderPassCreateInfo({}, attachments, subpass, dependency));
}
void VulkanPipeline::add_descriptor(const vk::DescriptorType type, vk::DescriptorBufferInfo buffer_info, const uint32_t binding, vk::ShaderStageFlags stages, const uint32_t pool_amount, const uint32_t desc_amount) {
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
			break;
		default:
			throw std::runtime_error("Called add_descriptor with image info, but type specified isn't a combined image sampler");
			break;
	}

	bindings.emplace_back(binding, type, desc_amount, stages, nullptr);

	add_to_pool(type, pool_amount);

	descriptor_writes.emplace_back(nullptr, binding, 0, type, image_info, nullptr, nullptr);
}
void VulkanPipeline::add_descriptor(const vk::DescriptorSetLayoutBinding binding, const vk::DescriptorPoolSize pool_size, const vk::WriteDescriptorSet write) {
	bindings.emplace_back(binding);
	descriptor_writes.push_back(write);
}
void VulkanPipeline::add_push_constant(const vk::PushConstantRange range) {
	push_constants.push_back(range);
}
void VulkanPipeline::build_descriptors() {
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
void VulkanPipeline::create_cache() {
	cache = device.createPipelineCacheUnique(vk::PipelineCacheCreateInfo());
}
void VulkanPipeline::create_pipeline() {
	build_descriptors();
	create_info.setStages(shader_stages);
	create_info.layout = *layout;
	create_info.renderPass = *render_pass;
	pipeline = device.createGraphicsPipelineUnique(*cache, create_info);
}
void RasterPipeline::build_pipeline(vk::Device device, vk::Format format, vk::Extent2D extent) {
	this->device = device;

	// Shaders
	auto v_code = read_file("resources/shaders/general.vert.spv");
	auto v_shader = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, v_code.size(), reinterpret_cast<uint32_t *>(v_code.data())));
	shader_stages.push_back(vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, *v_shader, "main"));
	auto f_code = read_file("resources/shaders/general.frag.spv");
	auto f_shader = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, f_code.size(), reinterpret_cast<uint32_t *>(f_code.data())));
	shader_stages.push_back(vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, *f_shader, "main"));

	// Render pass
	std::vector<vk::AttachmentDescription> attachments;
	vk::AttachmentDescription raster_attachment(
			{},
			format,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eLoad,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eGeneral,
			vk::ImageLayout::ePresentSrcKHR);
	attachments.push_back(raster_attachment);

	vk::AttachmentReference color_ref(0U, vk::ImageLayout::eColorAttachmentOptimal);
	vk::SubpassDependency dependency(
			VK_SUBPASS_EXTERNAL,
			0,
			{ vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests },
			{ vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests },
			{},
			{ vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite },
			{});
	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, nullptr, color_ref, nullptr, nullptr, nullptr);
	create_render_pass(attachments, subpass, dependency);

	create_cache();

	vk::PipelineVertexInputStateCreateInfo vertex_input{};
	auto desc = Vertex::get_attribute_descriptions();
	vertex_input.vertexAttributeDescriptionCount = desc.size();
	vertex_input.pVertexAttributeDescriptions = desc.data();
	auto bindings = Vertex::get_binding_description();
	vertex_input.vertexBindingDescriptionCount = bindings.size();
	vertex_input.pVertexBindingDescriptions = bindings.data();
	create_info.pVertexInputState = &vertex_input;

	vk::PipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.setTopology(vk::PrimitiveTopology::eTriangleList);
	input_assembly.primitiveRestartEnable = false;
	create_info.pInputAssemblyState = &input_assembly;

	vk::PipelineViewportStateCreateInfo viewport_state{};
	vk::Rect2D scissor({ 0, 0 }, extent);
	viewport_state.setScissors(scissor);
	vk::Viewport viewport(0.0f, 0.0f, extent.width, extent.height, 0.0f, 1.0f);
	viewport_state.setViewports(viewport);
	create_info.pViewportState = &viewport_state;

	vk::PipelineRasterizationStateCreateInfo rasterization{};
	rasterization.depthClampEnable = false;
	rasterization.rasterizerDiscardEnable = false;
	rasterization.polygonMode = vk::PolygonMode::eFill;
	rasterization.cullMode = vk::CullModeFlagBits::eNone;
	rasterization.frontFace = vk::FrontFace::eCounterClockwise;
	rasterization.depthBiasEnable = false;
	rasterization.lineWidth = 1.0f;
	create_info.pRasterizationState = &rasterization;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.sampleShadingEnable = false;
	multisampling.minSampleShading = 0.f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = false;
	multisampling.alphaToOneEnable = false;
	create_info.pMultisampleState = &multisampling;

	vk::PipelineDepthStencilStateCreateInfo depth{};
	depth.depthTestEnable = true;
	depth.depthWriteEnable = true;
	depth.depthCompareOp = vk::CompareOp::eLess;
	depth.depthBoundsTestEnable = false;
	depth.stencilTestEnable = false;
	create_info.pDepthStencilState = &depth;

	vk::PipelineColorBlendStateCreateInfo color_blend{};
	color_blend.logicOpEnable = false;
	color_blend.logicOp = vk::LogicOp::eCopy;
	vk::PipelineColorBlendAttachmentState blend_attachement{};
	blend_attachement.colorWriteMask = { vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
	blend_attachement.blendEnable = false;
	color_blend.attachmentCount = 1;
	color_blend.pAttachments = &blend_attachement;
	create_info.pColorBlendState = &color_blend;

	create_pipeline();
}
