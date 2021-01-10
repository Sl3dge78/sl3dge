#pragma once

#include <glm/glm.hpp>

struct CameraMatrices {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::mat4 view_inverse;
	alignas(16) glm::mat4 proj_inverse;
};

struct SunLight {
	alignas(16) glm::vec3 light_direction = glm::normalize(glm::vec3(-1.f, -1.f, -1.f));
	alignas(16) glm::vec3 light_color = { 1.f, 1.f, 1.f };
	alignas(4) float strength = 1.f;
};

struct PointLight {
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 color;
	alignas(4) float strength;
};

struct FragUniformBufferObject {
	alignas(16) glm::vec3 view_position;

	alignas(16) SunLight sun_light;

	alignas(4) float ambient_strength;
	alignas(4) float diffuse_strength;
	alignas(4) float specular_strength;
	alignas(4) float shininess;
};
