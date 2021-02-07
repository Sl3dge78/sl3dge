#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include "imgui/imgui.h"

#include "scene/Camera.h"
#include "scene/Material.h"
#include "scene/Mesh.h"
#include "vulkan/VulkanHelper.h"

class VulkanApplication;

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

struct Light {
	alignas(4) int32_t type;
	alignas(16) glm::vec3 color;
	alignas(4) float intensity;
	alignas(16) glm::vec3 vec;
	alignas(4) int32_t cast_shadows;
};

class Scene {
private:
	VulkanApplication *app;

	std::unique_ptr<Buffer> rtx_instances_buffer;
	std::unique_ptr<AccelerationStructure> tlas;

	void build_BLAS(vk::BuildAccelerationStructureFlagsKHR flags);
	void build_TLAS(vk::BuildAccelerationStructureFlagsKHR flags, bool update = false);
	void allocate_uniform_buffer();
	void refresh_materials();
	void refresh_lights();
	void refresh_scene_desc();

public:
	Camera camera;
	std::vector<std::unique_ptr<Mesh>> meshes;
	std::vector<MeshInstance> instances;
	std::vector<Material> materials;
	std::vector<Light> lights;
	std::vector<std::unique_ptr<Texture>> textures;
	vk::DeviceSize instances_size;
	vk::DeviceSize materials_size;
	vk::DeviceSize lights_size;

	std::unique_ptr<Buffer> scene_desc_buffer;
	std::unique_ptr<Buffer> materials_buffer;
	std::unique_ptr<Buffer> lights_buffer;

	bool draw_lights_ui = false;

	struct PushConstants {
		alignas(16) glm::vec3 view_pos;
		alignas(4) uint32_t light_count;
	} push_constants;

	Scene(VulkanApplication *app) :
			app(app){};

	void init();
	void update(float delta_time);

	uint32_t load_mesh(const std::string path);
	uint32_t create_material(const glm::vec3 &albedo = glm::vec3(0.5f, 0.5f, 0.5f), const float roughness = 0.5, const float metallic = 0, const float ao = 0, const uint32_t albedo_texture = -1);
	uint32_t load_texture(const std::string path);
	MeshInstance *create_instance(const uint32_t mesh_id, const uint32_t mat_id, glm::mat4 transform = glm::mat4(1.0f));
	uint32_t create_light(const int32_t type = 1, const glm::vec3 color = glm::vec3(1.0f), const float intensity = 1.0f, const glm::vec3 vec = glm::vec3(0.0f), const bool cast_shadows = false);

	vk::AccelerationStructureKHR get_tlas() { return tlas->get_acceleration_structure(); }
	void draw(vk::CommandBuffer cmd);
	void draw_menu_bar();
};

#endif //SCENE_H
