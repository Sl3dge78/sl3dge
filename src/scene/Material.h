#ifndef MATERIAL_H
#define MATERIAL_H
#pragma once

#include <memory>

#include <SDL/SDL.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "vulkan/VulkanHelper.h"

class VulkanApplication;

class Texture {
public:
	Texture(VulkanApplication &app, const std::string &texture_path);
	std::unique_ptr<Image> texture;
};
class Material {
public:
	alignas(4) float ambient_intensity;
	alignas(16) glm::vec3 diffuse_color;
	alignas(4) uint32_t texture_id;
	Material(const float ambient_intensity, const glm::vec3 &diffuse_color, const uint32_t texture_id) :
			ambient_intensity(ambient_intensity),
			diffuse_color(diffuse_color),
			texture_id(texture_id){};
};

#endif // !MATERIAL_H
