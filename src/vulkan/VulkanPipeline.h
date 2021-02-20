#pragma once

#include <vulkan/vulkan.hpp>

// TODO : c'est vraiment, vraiment dégueulasse ce truc; on tue l'inventeur de la poo a chaque ligne

class VulkanPipeline {
private:
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
	vk::PipelineBindPoint bind_point; // Ca c'est degueu
	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;

	vk::UniquePipeline pipeline;

	VulkanPipeline() = default;
	~VulkanPipeline();

	vk::PipelineLayout get_layout() { return *layout; }
	vk::PipelineCache get_cache() { return *cache; }

	void add_shader(const std::string path, const vk::ShaderStageFlagBits stage);
	void create_cache(vk::Device device);
	virtual void create_pipeline(vk::Device device) = 0; // Ca c'est degueu

public:
	vk::Pipeline get_pipeline() { return *pipeline; }

	void add_descriptor(const vk::DescriptorType type, const vk::ArrayProxyNoTemporaries<const vk::DescriptorBufferInfo> &buffer_info, const uint32_t binding, vk::ShaderStageFlags stages, const uint32_t pool_amount = 1, const uint32_t desc_amount = 1);
	void add_descriptor(const vk::DescriptorType type, const vk::ArrayProxyNoTemporaries<const vk::DescriptorImageInfo> &image_info, const uint32_t binding, vk::ShaderStageFlags stages, const uint32_t pool_amount = 1, const uint32_t desc_amount = 1);
	void add_descriptor(const vk::DescriptorSetLayoutBinding binding, const vk::DescriptorPoolSize pool_size, const vk::WriteDescriptorSet write); // Override to add descriptors manually
	void build_descriptors(vk::Device device);
	void add_push_constant(const vk::PushConstantRange range);

	void bind(vk::CommandBuffer cmd);
	void bind_descriptors(vk::CommandBuffer cmd);
	void bind_push_constants(vk::CommandBuffer cmd, const uint32_t id, void *value);
};

class GraphicsPipeline : public VulkanPipeline {
public:
	GraphicsPipeline() = default;
	void build_pipeline(vk::Device device, vk::Extent2D extent, vk::RenderPass render_pass);

private:
	vk::GraphicsPipelineCreateInfo graphics_create_info{};

	// Inherited via VulkanPipeline
	virtual void create_pipeline(vk::Device device) override;
};

class RaytracingPipeline : public VulkanPipeline {
public:
	std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups;
	vk::RayTracingPipelineCreateInfoKHR create_info;

	RaytracingPipeline() = default;
	void build_pipeline(vk::Device device);

	// Inherited via VulkanPipeline
	virtual void create_pipeline(vk::Device device) override;
};
