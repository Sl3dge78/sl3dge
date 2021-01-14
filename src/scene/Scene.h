//
// Created by Sledge on 2021-01-10.
//

#ifndef VULKAN_SCENE_H
#define VULKAN_SCENE_H

#include <vector>

#include "scene/Camera.h"
#include "scene/Mesh.h"
#include "vulkan/VulkanHelper.h"

class VulkanApplication;

struct MeshInstance {
	//Mesh *mesh;

	alignas(4) uint32_t mesh_id;
	alignas(16) glm::mat4 transform;
	alignas(16) glm::mat4 inverted;

	MeshInstance(const uint32_t mesh_id) :
			mesh_id(mesh_id) {
		transform = glm::mat4(1.0f);
		inverted = glm::inverse(transform);
	};
	void translate(glm::vec3 translation) {
		transform = glm::translate(transform, translation);
		inverted = glm::inverse(transform);
	}

	void rotate(const float angle, const glm::vec3 dir) {
		transform = glm::rotate(transform, angle, dir);
		inverted = glm::inverse(transform);
	}
};

class Scene {
private:
	std::unique_ptr<Buffer> rtx_instances_buffer;
	std::unique_ptr<AccelerationStructure> tlas;

public:
	Camera camera;
	std::vector<std::unique_ptr<Mesh>> meshes;
	std::vector<MeshInstance> instances;
	vk::DeviceSize instances_size;

	std::unique_ptr<Buffer> scene_desc_buffer;

	void allocate_uniform_buffer(VulkanApplication &app);

	void load_mesh(const std::string path, VulkanApplication *app);
	MeshInstance *create_instance(const uint32_t mesh_id, glm::mat4 transform = glm::mat4(1.0f));
	void build_BLAS(VulkanApplication &app, vk::BuildAccelerationStructureFlagsKHR flags);
	void build_TLAS(VulkanApplication &app, vk::BuildAccelerationStructureFlagsKHR flags, bool update = false);
	vk::AccelerationStructureKHR get_tlas() { return tlas->get_acceleration_structure(); }

	void rasterize(VulkanFrame &frame);
};

#endif //VULKAN_SCENE_H
