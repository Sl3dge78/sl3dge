//
// Created by Sledge on 2021-01-03.
//

#ifndef VULKAN_MESH_H
#define VULKAN_MESH_H

#include <vulkan/vulkan.hpp>

#include "tiny_obj_loader.h"

#include "Debug.h"
#include "Vertex.h"
#include "VulkanHelper.h"

class VulkanApplication;
class VulkanFrame;

struct MeshUBO {
	alignas(16) glm::mat4 transform;
};

class Mesh {
private:
	std::unique_ptr<Buffer> vertex_buffer;
	std::unique_ptr<Buffer> index_buffer;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	void update_matrix();

	static int get_id();
	inline static int id_ = 0;

	vk::UniqueAccelerationStructureKHR blas;
	std::unique_ptr<Buffer> blas_buffer;
	vk::DeviceAddress blas_address;

	vk::AccelerationStructureGeometryKHR geometry;
	vk::AccelerationStructureBuildRangeInfoKHR range_info;
	vk::AccelerationStructureBuildGeometryInfoKHR build_info;
	vk::AccelerationStructureGeometryTrianglesDataKHR triangles;

public:
	int id;
	bool show_window = false;
	glm::mat4 transform = glm::mat4(1.0f);

	Mesh(const std::string path);
	~Mesh() = default;

	void load(VulkanApplication *app);
	void draw(VulkanFrame &frame);
	void update(const float delta_time);

	vk::DeviceSize create_as(vk::Device device, vk::PhysicalDevice physical_device);
	void build_as(vk::Device device, vk::DeviceAddress scratch_address, vk::CommandBuffer cmd_buf);
	vk::DeviceAddress get_AS_reference() { return blas_address; }

	glm::vec3 position = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 rotation = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
};

#endif //VULKAN_MESH_H
