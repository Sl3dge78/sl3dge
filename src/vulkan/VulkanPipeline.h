#pragma once

#include <vulkan/vulkan.hpp>

class VulkanPipeline {
private:
	vk::UniquePipeline pipeline;
	vk::UniqueRenderPass render_pass;

	std::vector<vk::PushConstantRange> push_constants;
	vk::UniquePipelineLayout layout;
	vk::UniqueDescriptorSet set;
	vk::UniqueDescriptorSetLayout set_layout;
	vk::UniqueDescriptorPool pool;

	vk::UniquePipelineCache cache;

	std::vector<vk::WriteDescriptorSet> descriptor_writes;
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	std::vector<vk::DescriptorPoolSize> pool_sizes;

	void add_to_pool(const vk::DescriptorType type, const uint32_t count);

protected:
	vk::Device device;
	vk::GraphicsPipelineCreateInfo create_info{};
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;

	VulkanPipeline() = default;
	virtual ~VulkanPipeline() = default;

	void add_shader(const std::string path, const vk::ShaderStageFlagBits stage);
	void create_render_pass(std::vector<vk::AttachmentDescription> attachments, vk::SubpassDescription subpass, vk::SubpassDependency dependency);
	void build_descriptors();
	void create_cache();
	void create_pipeline();

public:
	void add_descriptor(const vk::DescriptorType type, vk::DescriptorBufferInfo buffer_info, const uint32_t binding, vk::ShaderStageFlags stages, const uint32_t pool_amount = 1, const uint32_t desc_amount = 1);
	void add_descriptor(const vk::DescriptorType type, const vk::ArrayProxyNoTemporaries<const vk::DescriptorImageInfo> &image_info, const uint32_t binding, vk::ShaderStageFlags stages, const uint32_t pool_amount = 1, const uint32_t desc_amount = 1);
	void add_descriptor(const vk::DescriptorSetLayoutBinding binding, const vk::DescriptorPoolSize pool_size, const vk::WriteDescriptorSet write); // Override to add descriptors manually
	void add_push_constant(const vk::PushConstantRange range);
	virtual void build_pipeline(vk::Device device, vk::Format format, vk::Extent2D extent) = 0;
};

class RasterPipeline : public VulkanPipeline {
public:
	RasterPipeline() = default;
	void build_pipeline(vk::Device device, vk::Format format, vk::Extent2D extent) override;
};
