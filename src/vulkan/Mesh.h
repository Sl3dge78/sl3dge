#ifndef VULKAN_MESH_H
#define VULKAN_MESH_H

#include <vulkan/vulkan.hpp>

#include "tiny_obj_loader.h"

#include "Debug.h"
#include "Vertex.h"
#include "vulkan/VulkanHelper.h"

class VulkanApplication;
class VulkanFrame;

class Mesh {
private:
	uint32_t id;
	vk::AccelerationStructureGeometryTrianglesDataKHR triangles;
	std::unique_ptr<Buffer> vertex_buffer;
	vk::DeviceSize vertex_size;
	std::unique_ptr<Buffer> index_buffer;
	vk::DeviceSize index_size;

	uint32_t next_instance_id;

protected:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

public:
	std::unique_ptr<AccelerationStructure> blas;
	vk::AccelerationStructureGeometryKHR geometry;
	vk::AccelerationStructureBuildRangeInfoKHR range_info;

	Mesh(VulkanApplication &app, const std::string path, const uint32_t id);
	~Mesh() = default;

	uint32_t get_id() const { return id; };
	uint32_t get_next_instance_id() { return next_instance_id++; };
	void bind(vk::CommandBuffer cmd);
	uint32_t get_primitive_count() const { return indices.size() / 3; };
	uint32_t get_index_count() const { return indices.size(); };
	vk::Buffer get_vtx_buffer() const { return vertex_buffer->buffer; };
	vk::Buffer get_idx_buffer() const { return index_buffer->buffer; };
};

#endif //VULKAN_MESH_H
