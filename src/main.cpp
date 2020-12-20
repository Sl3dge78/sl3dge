#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define TINYOBJLOADER_IMPLEMENTATION

#include <unordered_map>

#include <SDL/SDL.h>
#include <tiny_obj_loader.h>

#include "VulkanApplication.h"

#include "Camera.h"

class Sl3dge : public VulkanApplication {
private:
	Camera camera;
	bool show_demo_window = false;

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
		SDL_GetRelativeMouseState(NULL, NULL); // Called here to avoid the weird jump
		camera.start();
		transform.model = glm::mat4(1.0f);
		camera.get_view_matrix(transform.view);
		transform.proj = glm::perspective(glm::radians(80.f), float(swapchain_extent.width) / float(swapchain_extent.height), 0.1f, 1000.f);
		transform.proj[1][1] *= -1;
	}

	void update(float delta_time) override {
		if (Input::get_key_down(SDL_SCANCODE_F11))
			show_demo_window = !show_demo_window;

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		camera.update(delta_time);
		camera.get_view_matrix(transform.view);
	}
};

int main(int argc, char *argv[]) {
	Sl3dge app;

	try {
		app.run();
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
