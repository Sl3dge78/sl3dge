#pragma once

#include <array>
#include <functional>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 tex_coord;

	bool operator==(const Vertex &other) const {
		return pos == other.pos && normal == other.normal && tex_coord == other.tex_coord;
	}

	static VkVertexInputBindingDescription get_binding_description() {
		VkVertexInputBindingDescription description{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};

		return description;
	}

	static std::array<VkVertexInputAttributeDescription, 3> get_attribute_descriptions() {
		std::array<VkVertexInputAttributeDescription, 3> descriptions{};
		descriptions[0].location = 0;
		descriptions[0].binding = 0;
		descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		descriptions[0].offset = offsetof(Vertex, pos);

		descriptions[1].location = 1;
		descriptions[1].binding = 0;
		descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		descriptions[1].offset = offsetof(Vertex, normal);

		descriptions[2].location = 2;
		descriptions[2].binding = 0;
		descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		descriptions[2].offset = offsetof(Vertex, tex_coord);

		return descriptions;
	}
};
namespace std {
template <>
struct hash<Vertex> {
	size_t operator()(Vertex const &vertex) const {
		return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.tex_coord) << 1);
	}
};
} // namespace std
