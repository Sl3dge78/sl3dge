//
// Created by Sledge on 2021-01-10.
//

#ifndef VULKAN_SCENE_H
#define VULKAN_SCENE_H

#include <vector>

#include "Mesh.h"
#include "vulkan/VulkanHelper.h"

class VulkanApplication;

struct MeshInstance {
	Mesh *mesh;
	glm::mat4 transform = glm::mat4(1.0f);

	MeshInstance(Mesh *mesh) :
			mesh(mesh){};
	void translate(glm::vec3 translation) {
		transform = glm::translate(transform, translation);
	}
};

class Scene {
private:
	std::vector<std::unique_ptr<Mesh>> meshes;
	std::unique_ptr<Buffer> rtx_instances_buffer;
	std::unique_ptr<AccelerationStructure> tlas;

public:
	std::vector<std::unique_ptr<MeshInstance>> instances;

	void load_mesh(const std::string path, VulkanApplication *app);
	MeshInstance *create_instance(const uint32_t mesh_id, glm::mat4 transform = glm::mat4(1.0f));
	void build_BLAS(VulkanApplication &app, vk::BuildAccelerationStructureFlagsKHR flags);
	void build_TLAS(VulkanApplication &app, vk::BuildAccelerationStructureFlagsKHR flags, bool update = false);
	vk::AccelerationStructureKHR get_tlas() { return tlas->get_acceleration_structure(); }

	void rasterize(VulkanFrame &frame);
};

#endif //VULKAN_SCENE_H
