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

	vk::AccelerationStructureGeometryTrianglesDataKHR triangles;

public:
	int id;
	bool show_window = false;
	glm::mat4 transform = glm::mat4(1.0f);

	std::unique_ptr<AccelerationStructure> blas;
	vk::AccelerationStructureGeometryKHR geometry;
	vk::AccelerationStructureBuildRangeInfoKHR range_info;

	Mesh(const std::string path, VulkanApplication &app);
	~Mesh() = default;

	void draw(VulkanFrame &frame);
	void update(const float delta_time);

	uint32_t get_primitive_count() { return indices.size() / 3; }

	glm::vec3 position = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 rotation = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
};

#endif //VULKAN_MESH_H
