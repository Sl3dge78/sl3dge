#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define TINYOBJLOADER_IMPLEMENTATION

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <deque>
#include <unordered_map>

#include <SDL/SDL.h>

#include "Camera.h"
#include "Mesh.h"
#include "vulkan/VulkanApplication.h"

class Sl3dge : public VulkanApplication {
private:
	Camera camera;
	bool show_demo_window = false;
	bool show_metrics = false;
	bool show_lighting_options = false;

	glm::vec3 light_position = glm::vec3(0.0f, 0.0f, 0.5f);

	void load(std::vector<std::unique_ptr<Mesh>> &meshes) override {
		meshes.emplace_back(std::make_unique<Mesh>("resources/models/viking_room.obj"));
		meshes.emplace_back(std::make_unique<Mesh>("resources/models/viking_room.obj"));
		SDL_Log("Loaded %d meshes", meshes.size());
	}

	void start() override {
		SDL_GetRelativeMouseState(nullptr, nullptr); // Called here to avoid the weird jump
		camera.start();
		camera.get_view_matrix(scene_info_ubo.view);
		scene_info_ubo.proj = glm::perspective(glm::radians(80.f), get_aspect_ratio(), 0.1f, 1000.f);
		scene_info_ubo.proj[1][1] *= -1;

		fubo.ambient_strength = .1f;
		fubo.diffuse_strength = .5f;
		fubo.specular_strength = .5f;
		fubo.shininess = 8.0f;
	}

	void update(float delta_time) override {
		camera.update(delta_time);
		int i = 0;
		for (auto &mesh : meshes) {
			ImGui::PushID(i);
			mesh->update(delta_time);
			ImGui::PopID();
			i++;
		}

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Options")) {
				ImGui::MenuItem("Display Lighting Options", "F10", &show_lighting_options);
				ImGui::MenuItem("Display Metrics", "F11", &show_metrics);
				ImGui::MenuItem("Cam Info", "", &camera.show_window);
				ImGui::MenuItem("RTX", "F9", &this->rtx);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::BeginMenu("Meshes")) {
				for (auto &mesh : meshes) {
					ImGui::MenuItem("Info", "", &mesh->show_window);
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			ImGui::Text("%.1f FPS", 1.f / delta_time);
		}
		ImGui::EndMainMenuBar();

		if (Input::get_key_down(SDL_SCANCODE_F10))
			show_lighting_options = !show_lighting_options;
		if (Input::get_key_down(SDL_SCANCODE_F11))
			show_metrics = !show_metrics;
		if (Input::get_key_down(SDL_SCANCODE_F9))
			rtx = !rtx;

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
		if (show_metrics)
			show_metrics_window(show_metrics, delta_time);
		if (show_lighting_options)
			show_lighting_window(show_lighting_options);

		camera.get_view_matrix(scene_info_ubo.view);
		fubo.view_position = camera.get_position();
	}

	void show_metrics_window(bool &opened, float delta_time) {
		static std::vector<float> frames;
		frames.push_back(1.f / delta_time);
		if (frames.size() > 100)
			frames.erase(frames.begin());

		ImGui::SetNextWindowPos(ImVec2(10.f, 50.f), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.35f);
		if (!ImGui::Begin("metrics", &show_metrics, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
			ImGui::End();
			return;
		}
		//ImGui::Text("%d vtx, %d idx (%d tri)", vertices.size(), indices.size(), indices.size() / 3);
		ImGui::PlotLines("FPS", frames.data(), frames.size(), 0, nullptr, 50.0f, 60.0f, ImVec2(0, 80.0f), 4);

		ImGui::End();
	}

	void show_lighting_window(bool &opened) {
		if (!ImGui::Begin("Lighting", &show_lighting_options, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::End();
			return;
		}

		ImGui::InputFloat3("Sun Direction", &fubo.sun_light.light_direction[0]);
		fubo.sun_light.light_direction = glm::normalize(fubo.sun_light.light_direction);
		ImGui::ColorEdit3("Sun Color", &fubo.sun_light.light_color[0]);
		ImGui::SliderFloat("Sun Intensity", &fubo.sun_light.strength, 0.f, 1.f);
		ImGui::Separator();
		ImGui::SliderFloat("Ambient Strength", &fubo.ambient_strength, 0.f, 1.f);
		ImGui::SliderFloat("Diffuse Strength", &fubo.diffuse_strength, 0.f, 1.f);
		ImGui::SliderFloat("Specular Strength", &fubo.specular_strength, 0.f, 1.f);
		ImGui::InputFloat("Shininess", &fubo.shininess);
		ImGui::End();
	}
};

int main(int argc, char *argv[]) {
	Sl3dge app;
	app.run();

	return EXIT_SUCCESS;
}
