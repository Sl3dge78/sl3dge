//
// Created by Sledge on 2021-01-03.
//

#ifndef VULKAN_MESH_H
#define VULKAN_MESH_H

#include <vulkan/vulkan.hpp>

#include "tiny_obj_loader.h"

#include "VulkanHelper.h"
#include "Vertex.h"

class VulkanApplication;

class Mesh {
	std::unique_ptr<Buffer> vertex_buffer;
	std::unique_ptr<Buffer> index_buffer;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

public:
	Mesh(const std::string path);
	void load(VulkanApplication *app);
	void draw(vk::CommandBuffer cmd_buf);

	vk::AccelerationStructureGeometryKHR geometry;
	vk::AccelerationStructureBuildRangeInfoKHR range_info;
};

#endif //VULKAN_MESH_H
