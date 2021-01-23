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
	alignas(4) float ambient_strength;
	alignas(4) float shininess;
	alignas(4) float specular_strength;
	alignas(16) glm::vec3 diffuse_color;
	alignas(4) int32_t texture_id;
	Material(const float ambient_intensity, const glm::vec3 &diffuse_color, const float shininess, const float specular_strength, const uint32_t texture_id) :
			ambient_strength(ambient_intensity), shininess(shininess), specular_strength(specular_strength), diffuse_color(diffuse_color), texture_id(texture_id){};
};

#endif // !MATERIAL_H
