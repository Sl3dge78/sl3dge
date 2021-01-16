#ifndef MATERIAL_H
#define MATERIAL_H
#pragma once

#include <memory>

#include <SDL/SDL.h>
#include <vulkan/vulkan.hpp>

#include "vulkan/VulkanHelper.h"

class VulkanApplication;

class Material {
public:
	Material(VulkanApplication &app, const std::string &texture_path);
	std::unique_ptr<Image> texture;
};

#endif // !MATERIAL_H
