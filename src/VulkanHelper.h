#pragma once

#include <fstream>

#include <vector>

#include <vulkan/vulkan.h>

#include <SDL/SDL.h>

#include "Debug.h"

const std::vector<const char *> validation_layers = { "VK_LAYER_KHRONOS_validation" };

std::vector<char> read_file(const std::string &path);

VkFormat get_vk_format(SDL_PixelFormat *format);

void check_vk_result(VkResult err);

uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties);

void create_image_view(VkDevice device, VkImage &image, VkFormat format, VkImageAspectFlags aspect, VkImageView *image_view);

void create_buffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory);

bool check_validation_layer_support();

std::vector<const char *> get_required_extensions(SDL_Window *window);
