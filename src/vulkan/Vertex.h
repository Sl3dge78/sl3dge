#pragma once

#include <array>
#include <functional>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

struct Vertex {
	alignas(16) glm::vec3 pos;
	alignas(16) glm::vec3 normal;
	alignas(16) glm::vec2 tex_coord;

	bool operator==(const Vertex &other) const {
		return pos == other.pos && normal == other.normal && tex_coord == other.tex_coord;
	}

	static const std::vector<vk::VertexInputBindingDescription> get_binding_description() {
		std::vector<vk::VertexInputBindingDescription> ret = {
			vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex)
		};
		return ret;
	}

	static const std::vector<vk::VertexInputAttributeDescription> get_attribute_descriptions() {
		std::vector<vk::VertexInputAttributeDescription> ret = {
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
			vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, tex_coord)),
		};

		return ret;
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
