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
	glm::vec3 albedo;
	int32_t albedo_texture_id;
	float roughness;
	float metallic;
	float ao;
	float rim_pow;
	float rim_strength;

	bool draw_ui = false;

private:
	uint32_t id = 0;

public:
	Material(const uint32_t id_, const glm::vec3 &albedo = glm::vec3(0.5f, 0.5f, 0.5f), const float roughness = 0.5, const float metallic = 0, const float ao = 0, const uint32_t albedo_texture = -1) {
		this->id = id_;
		this->albedo = albedo;
		this->albedo_texture_id = albedo_texture;
		this->metallic = metallic;
		this->roughness = roughness;
		this->ao = ao;
		this->rim_pow = 16;
		this->rim_strength = 2;
	};
	bool on_gui();
	uint32_t get_id() const {
		return id;
	}
};

#endif // !MATERIAL_H
