#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define TINYOBJLOADER_IMPLEMENTATION

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <cstdlib>
#include <ctime>
#include <deque>
#include <unordered_map>

#include <SDL/SDL.h>

#include "Input.h"
#include "nodes/MeshInstance.h"
#include "scene/Material.h"
#include "scene/Scene.h"
#include "vulkan/VulkanApplication.h"

// == FEATURES ==
// TODO : Collision avec le sol + gravité

// == AMELIO ==
// TODO : gros clean
// TODO : Couper vulkan app en 2 1 pour rtx et un pour raster ?
// TODO : Textures for metallic, roughness, ao & normal
// TODO : Image based lighting https://learnopengl.com/PBR/IBL/Diffuse-irradiance
// TODO : Add ui to move instances
// TODO : Single vertex and index buffer. The instances have an offset in these tables. Then draw indexed instanced.
// TODO : GLTF
// TODO : GI in RayTracing			// Different pipelines ?
// TODO : Shadows in RayTracing		// Different pipelines ?
// TODO : Update TLAS when an object has moved
// TODO - AMELIO : Virer VulkanFrame et le gérer dans la swapchain directement ? ou virer swapchain et le gerer dans l'app? C'est de la complexité inutile en trop

class Sl3dge : public VulkanApplication {
private:
	float time = 0.0f;

	void make_sphere() {
		/*
		float x = float(std::rand() % 10);
		float y = float(std::rand() % 10);
		sphere_a->translate(glm::vec3(x, y, 1.0f));
		*/
	}
	void load() override {
		scene->camera.load(this);
		auto viking_texture = scene->load_texture("resources/textures/viking_room.png"); // 0
		/*
		auto viking_mesh = scene->load_mesh(this, "resources/models/viking_room.obj"); // 0
		auto viking_material = scene->create_material(0.1f, glm::vec3(1.0f, 1.0f, 1.0f), viking_texture);

		auto a = scene->create_instance(viking_mesh, viking_material);
		a->translate(glm::vec3(0.f, 1.f, 0.f));
		
		auto b = scene->create_instance(viking_mesh, viking_material);
		b->translate(glm::vec3(1.5f, 1.f, 0.f));
		b->rotate(3.14f, glm::vec3(0.f, 0.f, 1.f));
		*/
		/*
		auto sphere_mesh = scene->load_mesh("resources/models/sphere.obj");
		auto sphere_material = scene->create_material(glm::vec3(1.0f, 1.0f, 0.0f), 1.f);
		auto sphere_a = scene->create_instance(sphere_mesh, sphere_material);
		sphere_a->translate(glm::vec3(0.0f, 0.0f, 1.0f));

		auto plane_mesh = scene->load_mesh("resources/models/plane.obj");
		auto plane_material = scene->create_material(glm::vec3(.5f, .5f, .5f), 0.1f);
		auto plane_a = scene->create_instance(plane_mesh, plane_material);
		*/
		/*
		auto dune_mesh = scene->load_mesh("resources/models/dune.obj");
		auto dune_material = scene->create_material(glm::vec3(255.f / 255.f, 187.f / 255.f, 79.f / 255.f), 0.1f, 0.0f, 0.1f);
		auto dune_instance = scene->create_instance(dune_mesh, dune_material);
		*/

		Mesh *dune_mesh = scene->load_mesh("resources/models/dune.obj");
		Mesh *sphere_mesh = scene->load_mesh("resources/models/sphere.obj");

		Material *dune_material = scene->create_material(glm::vec3(255.f / 255.f, 187.f / 255.f, 79.f / 255.f), 0.1f, 0.0f, 0.1f);
		Material *sphere_material = scene->create_material(glm::vec3(1.0f, 1.0f, 1.0f), 0.1f);

		MeshInstance *dune = scene->create_node<MeshInstance>(&scene->scene_root, dune_mesh, dune_material);
		MeshInstance *sphere = scene->create_node<MeshInstance>(&scene->scene_root, sphere_mesh, sphere_material);
		sphere->translate(glm::vec3(5.0f, 5.0f, 1.0f));
		MeshInstance *sphere2 = scene->create_node<MeshInstance>(&scene->scene_root, sphere_mesh, sphere_material);
		sphere2->translate(glm::vec3(7.0f, 5.0f, 1.0f));

		scene->create_light(0, glm::vec3(1.0), 1.0, glm::normalize(glm::vec3(1.0, 1.0, 0.15)), true);
		//scene->create_light(1, glm::vec3(1.0), 1.0, glm::vec3(0.0, 0.0, 3.0));
	}
	void start() override {
		SDL_GetRelativeMouseState(nullptr, nullptr); // Called here to avoid the weird jump
		scene->camera.start();

		scene->push_constants.clear_color = glm::vec4(0.43f, 0.77f, .91f, 1.f);
		scene->push_constants.light_dir = glm::normalize(glm::vec3(1.0f, 1.f, .15f));
		scene->push_constants.light_intensity = 5.0f;
		scene->push_constants.light_color = glm::vec3(.99, .72, 0.07);
	}
	void update(float delta_time) override {
		scene->update(delta_time);

		time += delta_time;
		const float speed = 1.0f / 10.0f;

		//rtx_push_constants.light_dir = glm::normalize(glm::vec3(glm::abs(glm::cos(time * speed)), 1.0f, glm::abs(glm::sin(time * speed))));
		scene->push_constants.light_dir = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Options")) {
				ImGui::MenuItem("Camera params", "", &scene->camera.show_window);
				ImGui::MenuItem("Refresh Shaders", "F5", nullptr);
				ImGui::MenuItem("Toggle rtx", "F6", nullptr);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			ImGui::Text("%.1f FPS", 1.f / delta_time);
		}

		ImGui::EndMainMenuBar();

		if (Input::get_key_down(SDL_SCANCODE_F5)) {
			this->refresh_shaders();
		}
		if (Input::get_key_down(SDL_SCANCODE_F6)) {
			this->toggle_rtx();
		}
	}
};

int main(int argc, char *argv[]) {
	Sl3dge app;
	app.run();
	return EXIT_SUCCESS;
}
