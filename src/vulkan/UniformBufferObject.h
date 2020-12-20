#pragma once

#include <glm/glm.hpp>

// Todo : wrapper class
struct VtxUniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct FragUniformBufferObject {
	alignas(16) glm::vec3 view_position;
	alignas(16) glm::vec3 light_position;
	alignas(16) glm::vec3 light_color;
};
