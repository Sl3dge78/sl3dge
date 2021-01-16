#ifndef VULKAN_MESH_H
#define VULKAN_MESH_H

#include <vulkan/vulkan.hpp>

#include "tiny_obj_loader.h"

#include "Debug.h"
#include "Vertex.h"
#include "vulkan/VulkanHelper.h"

class VulkanApplication;
class VulkanFrame;

struct MeshUBO {
	alignas(16) glm::mat4 transform;
};

class Mesh {
private:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	void update_matrix();

	static int get_id();
	inline static int id_ = 0;

	vk::AccelerationStructureGeometryTrianglesDataKHR triangles;

public:
	int id;
	bool show_window = false;

	std::unique_ptr<Buffer> vertex_buffer;
	vk::DeviceSize vertex_size;
	std::unique_ptr<Buffer> index_buffer;
	vk::DeviceSize index_size;

	glm::mat4 transform = glm::mat4(1.0f);

	std::unique_ptr<AccelerationStructure> blas;
	vk::AccelerationStructureGeometryKHR geometry;
	vk::AccelerationStructureBuildRangeInfoKHR range_info;

	Mesh(VulkanApplication &app, const std::string path);
	~Mesh() = default;

	void draw(VulkanFrame &frame);
	void update(const float delta_time);

	uint32_t get_primitive_count() { return indices.size() / 3; }

	glm::vec3 position = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 rotation = glm::vec3(0.f, 0.f, 0.f);
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
};

#endif //VULKAN_MESH_H
