#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define TINYOBJLOADER_IMPLEMENTATION

#include <deque>
#include <unordered_map>

#include <SDL/SDL.h>
#include <tiny_obj_loader.h>

#include "vulkan/VulkanApplication.h"

#include "Camera.h"

class Sl3dge : public VulkanApplication {
private:
	Camera camera;
	bool show_demo_window = false;
	bool show_metrics = false;
	bool show_lighting_options = false;

	glm::vec3 light_position = glm::vec3(0.0f, 0.0f, 0.5f);

	void load(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices) override {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "resources/models/viking_room.obj")) {
			SDL_LogError(0, "Unable to read model %s", err.c_str());
		}

		std::unordered_map<Vertex, uint32_t> unique_vtx{};

		for (const auto &shape : shapes) {
			for (const auto &index : shape.mesh.indices) {
				Vertex vertex{};
				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
				vertex.tex_coord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				if (unique_vtx.count(vertex) == 0) {
					unique_vtx[vertex] = uint32_t(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(unique_vtx[vertex]);
			}
		}
	}

	void start() override {
		SDL_GetRelativeMouseState(nullptr, nullptr); // Called here to avoid the weird jump
		camera.start();
		vubo.model = glm::mat4(1.0f);
		camera.get_view_matrix(vubo.view);
		vubo.proj = glm::perspective(glm::radians(80.f), get_aspect_ratio(), 0.1f, 1000.f);
		vubo.proj[1][1] *= -1;

		fubo.ambient_strength = .1f;
		fubo.diffuse_strength = .5f;
		fubo.specular_strength = .5f;
		fubo.shininess = 8.0f;
	}

	void update(float delta_time) override {
		camera.update(delta_time);

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("Options")) {
				ImGui::MenuItem("Display Lighting Options", "F10", &show_lighting_options);
				ImGui::MenuItem("Display Metrics", "F11", &show_metrics);
				ImGui::MenuItem("Cam Info", "", &camera.show_window);
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

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
		if (show_metrics)
			show_metrics_window(show_metrics, delta_time);
		if (show_lighting_options)
			show_lighting_window(show_lighting_options);

		camera.get_view_matrix(vubo.view);
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
		ImGui::Text("%d vtx, %d idx (%d tri)", vertices.size(), indices.size(), indices.size() / 3);
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
