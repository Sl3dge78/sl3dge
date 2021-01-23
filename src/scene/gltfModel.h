#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "vulkan/VulkanHelper.h"

class gltfModel {
public:
	struct gltf_Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec3 color;
	};

	std::unique_ptr<Buffer> vertices;
	std::unique_ptr<Buffer> indices;
	uint32_t indices_count;

	struct Primitive {
		uint32_t first_index;
		uint32_t index_count;
		int32_t material_id;
	};

	struct Mesh {
		std::vector<Primitive> primitives;
	};

	struct Node;

	struct Node {
		Node *parent;
		std::vector<Node> children;
		Mesh mesh;
		glm::mat4 matrix;
	};

	struct Material {
		glm::vec4 base_color_factor = glm::vec4(1.0f);
		uint32_t base_color_texture_id;
	};

	struct gltf_Image {
		std::unique_ptr<Image> texture;
		vk::DescriptorSet descriptor_set;
	};

	struct Texture {
		int32_t image_index;
	};

	std::vector<Image> images;
	std::vector<Texture> textures;
	std::vector<Material> materials;
	std::vector<Node> nodes;
};
