#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define TINYOBJLOADER_IMPLEMENTATION

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <deque>
#include <unordered_map>

#include <SDL/SDL.h>

#include "scene/Material.h"
#include "scene/Scene.h"
#include "vulkan/VulkanApplication.h"

// TODO : Virer VulkanFrame et le gérer dans la swapchain directement ? ou virer swapchain et le gerer dans l'app? C'est de la complexité inutile en trop
// TODO : GLTF
//		- Materials
//		- Textures
// TODO : Clean all this
// TODO : Materials v2 :
//		- Store a handle to a sampler
//		- Store a handle to a texture
//		- Multiple parameters (diffuse, etc)

class Sl3dge : public VulkanApplication {
private:
	void load() override {
		auto viking_mesh = scene->load_mesh(this, "resources/models/viking_room.obj"); // 0
		auto viking_texture = scene->load_material(this, "resources/textures/viking_room.png"); // 0

		auto a = scene->create_instance(viking_mesh, viking_texture);
		a->translate(glm::vec3(0.f, 1.f, 0.f));
		auto b = scene->create_instance(viking_mesh, viking_texture);
		b->translate(glm::vec3(1.5f, 1.f, 0.f));
		b->rotate(3.14f, glm::vec3(0.f, 0.f, 1.f));
		scene->camera.load(this);
	}

	void start() override {
		SDL_GetRelativeMouseState(nullptr, nullptr); // Called here to avoid the weird jump
		scene->camera.start();

		rtx_push_constants.clear_color = glm::vec4(0.1f, 0.1f, 0.1f, 1.f);
		rtx_push_constants.light_pos = glm::vec3(1.1f, 1.f, 1.f);
		rtx_push_constants.light_intensity = 0.5f;
		rtx_push_constants.light_type = 0;
	}
	void update(float delta_time) override {
		scene->camera.update(delta_time);

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Options")) {
				ImGui::EndMenu();
			}
			ImGui::Separator();
			ImGui::Text("%.1f FPS", 1.f / delta_time);
		}
		ImGui::EndMainMenuBar();
	}
};

int main(int argc, char *argv[]) {
	Sl3dge app;
	app.run();

	return EXIT_SUCCESS;
}
