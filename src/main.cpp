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
#include "game/Player.h"
#include "game/Terrain.h"
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
	Mesh *dune_mesh;
	Mesh *sphere_mesh;
	Material *dune_material;
	Material *sphere_material;
	Mesh *gizmo_mesh;
	Material *gizmo_mat;

	Terrain *terrain;
	Player *player;

	void load(Scene &scene) override {
		auto viking_texture = scene.load_texture("resources/textures/viking_room.png"); // 0

		dune_mesh = scene.load_mesh("resources/models/plane.obj");
		dune_material = scene.create_material(glm::vec3(255.f / 255.f, 187.f / 255.f, 79.f / 255.f), 0.1f, 0.0f, 0.1f);
		terrain = scene.create_node<Terrain>(&scene.scene_root, dune_mesh, dune_material, 128, 128);

		sphere_mesh = scene.load_mesh("resources/models/sphere.obj");
		sphere_material = scene.create_material(glm::vec3(1.0f, 1.0f, 1.0f), 0.1f);
		MeshInstance *sphere = scene.create_node<MeshInstance>(&scene.scene_root, sphere_mesh, sphere_material);
		sphere->translate(glm::vec3(5.0f, 5.0f, 1.0f));

		player = scene.create_node<Player>(&scene.scene_root);
		player->camera = scene.create_node<Camera>(player);
		player->camera->translate(glm::vec3(0.0, 0.0f, 1.8f));
		player->camera->rotate(-90.0f, player->camera->left());
		player->translate(glm::vec3(5.0, 0.0, 5.0));

		scene.main_camera = player->camera;
	}
	virtual void build_scene_graph(Scene &scene) override {
	}
	void start(Scene &scene) override {
		SDL_GetRelativeMouseState(nullptr, nullptr); // Called here to avoid the weird jump

		scene.push_constants.clear_color = glm::vec4(0.43f, 0.77f, .91f, 1.f);
		scene.push_constants.light_dir = glm::normalize(glm::vec3(1.0f, 1.f, .15f));
		scene.push_constants.light_intensity = 5.0f;
		scene.push_constants.light_color = glm::vec3(.99, .72, 0.07);
	}
	void update(Scene &scene, float delta_time) override {
		scene.update(delta_time);

		scene.push_constants.light_dir = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Options")) {
				ImGui::MenuItem("Camera params", "", &scene.main_camera->show_window);
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
