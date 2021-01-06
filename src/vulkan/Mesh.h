//
// Created by Sledge on 2021-01-03.
//

#ifndef VULKAN_MESH_H
#define VULKAN_MESH_H

#include <vulkan/vulkan.hpp>

#include "tiny_obj_loader.h"

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
	glm::mat4 transform = glm::mat4(1.0f);

	void update_matrix();

public:
	bool show_window = false;

	Mesh(const std::string path);
	void load(VulkanApplication *app);
	void draw(VulkanFrame &frame);
	void update(float delta_time);
	~Mesh() = default;

	vk::AccelerationStructureGeometryKHR geometry;
	vk::AccelerationStructureBuildRangeInfoKHR range_info;
	vk::UniqueAccelerationStructureKHR acceleration_structure;

	glm::vec3 position = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 rotation = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
};

#endif //VULKAN_MESH_H
