#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include "scene/Camera.h"
#include "scene/Material.h"
#include "scene/Mesh.h"
#include "vulkan/VulkanHelper.h"

class VulkanApplication;

// TODO : Single vertex and index buffer. The instances have an offset in these tables. Then draw indexed instanced

class MeshInstance {
private:
	alignas(4) uint32_t mesh_id; // Typedef that as mesh handle
	alignas(4) uint32_t mat_id; // typedef that as mat_handle
	alignas(16) glm::mat4 transform;
	alignas(16) glm::mat4 inverted;

public:
	MeshInstance(const uint32_t mesh_id, const uint32_t mat_id) :
			mesh_id(mesh_id), mat_id(mat_id) {
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
	glm::mat4 get_transform() const { return transform; }
	uint32_t get_mesh_id() const { return mesh_id; }
};

class Scene {
private:
	std::unique_ptr<Buffer> rtx_instances_buffer;
	std::unique_ptr<AccelerationStructure> tlas;

	void build_BLAS(VulkanApplication &app, vk::BuildAccelerationStructureFlagsKHR flags);
	void build_TLAS(VulkanApplication &app, vk::BuildAccelerationStructureFlagsKHR flags, bool update = false);
	void allocate_uniform_buffer(VulkanApplication &app);

public:
	Camera camera;
	std::vector<std::unique_ptr<Mesh>> meshes;
	std::vector<MeshInstance> instances;
	std::vector<Material> materials;
	std::vector<std::unique_ptr<Texture>> textures;
	vk::DeviceSize instances_size;
	vk::DeviceSize materials_size;

	std::unique_ptr<Buffer> scene_desc_buffer;
	std::unique_ptr<Buffer> materials_buffer;

	void init(VulkanApplication &app);

	uint32_t load_mesh(VulkanApplication *app, const std::string path);
	uint32_t create_material(const float ambient_intensity, const glm::vec3 diffuse_color, const int32_t texture_id = -1);
	uint32_t load_texture(VulkanApplication *app, const std::string path);
	MeshInstance *create_instance(const uint32_t mesh_id, const uint32_t mat_id, glm::mat4 transform = glm::mat4(1.0f));

	vk::AccelerationStructureKHR get_tlas() { return tlas->get_acceleration_structure(); }
	void draw(vk::CommandBuffer cmd);
};

#endif //SCENE_H
