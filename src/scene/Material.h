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
struct Material {
	alignas(16) glm::vec3 albedo;
	alignas(4) int32_t albedo_texture_id;
	alignas(4) float roughness;
	alignas(4) float metallic;
	alignas(4) float ao;

	bool draw_ui = false;

	Material(const glm::vec3 &albedo = glm::vec3(0.5f, 0.5f, 0.5f), const float roughness = 0.5, const float metallic = 0, const float ao = 0, const uint32_t albedo_texture = -1) {
		this->albedo = albedo;
		this->albedo_texture_id = albedo_texture;
		this->metallic = metallic;
		this->roughness = roughness;
		this->ao = ao;
	};
};

#endif // !MATERIAL_H
