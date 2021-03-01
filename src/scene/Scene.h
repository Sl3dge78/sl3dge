#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include "imgui/imgui.h"

#include "nodes/Camera.h"
#include "nodes/MeshInstance.h"
#include "nodes/Node.h"
#include "nodes/Node3D.h"
#include "scene/Material.h"
#include "vulkan/Mesh.h"
#include "vulkan/VulkanHelper.h"
#include "vulkan/VulkanPipeline.h"

class VulkanApplication;

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
	std::unique_ptr<Buffer> scene_desc_buffer;
	std::unique_ptr<Buffer> materials_buffer;
	std::unique_ptr<Buffer> lights_buffer;
	std::unique_ptr<Buffer> camera_buffer;

	std::vector<std::unique_ptr<Mesh>> meshes;
	std::vector<std::unique_ptr<Material>> materials;
	std::vector<Light> lights;

	vk::UniqueSampler texture_sampler;
	std::vector<std::unique_ptr<Texture>> textures;

	bool is_dirty = false;

	void build_BLAS(vk::BuildAccelerationStructureFlagsKHR flags);
	void build_TLAS(vk::BuildAccelerationStructureFlagsKHR flags, bool update = false);
	void upload_buffers(bool update = false);

public:
	Camera *main_camera;

	struct PushConstant {
		alignas(16) glm::vec4 clear_color;
		alignas(16) glm::vec3 light_dir;
		alignas(4) float light_intensity;
		alignas(16) glm::vec3 light_color;
	} push_constants;

	Node3D scene_root = Node3D(nullptr);
	std::vector<std::unique_ptr<Node>> nodes;

	Scene(VulkanApplication *app) :
			app(app){};

	void init();
	void write_descriptors(VulkanPipeline &pipeline);

	void start();
	void update(float delta_time);

	Mesh *load_mesh(const std::string path);
	Material *create_material(const glm::vec3 &albedo = glm::vec3(0.5f, 0.5f, 0.5f), const float roughness = 0.5, const float metallic = 0, const float ao = 0, const uint32_t albedo_texture = -1);
	uint32_t load_texture(const std::string path);
	uint32_t create_light(const int32_t type = 1, const glm::vec3 color = glm::vec3(1.0f), const float intensity = 1.0f, const glm::vec3 vec = glm::vec3(0.0f), const bool cast_shadows = false);

	template <class N, class... T>
	N *create_node(T... args);

	vk::AccelerationStructureKHR get_tlas() { return tlas->get_acceleration_structure(); }
	void draw(vk::CommandBuffer cmd);

	void set_dirty();

	/* EDITOR STUFF */
private:
	Node *selected_node = nullptr;
	bool show_scene_browser = true;
	bool show_node_info = true;

	void draw_gui();
	void draw_scene_browser();
	void draw_selected_node_info();

public:
	void draw_menu_bar();
};

template <class N, class... T>
inline N *Scene::create_node(T... args) {
	static_assert(std::is_base_of<Node, N>::value, "Trying to create a node that isn't derived from node!");
	std::unique_ptr<N> n = std::make_unique<N>(args...);
	n->scene = this;
	N *ret = n.get();
	nodes.push_back(std::move(n));
	return ret;
}

#endif //SCENE_H
