//
// Created by Sledge on 2021-01-03.
//

#include "Mesh.h"
#include "vulkan/VulkanApplication.h"

#include <string>

Mesh::Mesh(const std::string path, VulkanApplication &app) {
	this->id = get_id();

	// Todo : load this only once, and then point to the data for each mesh as reqs
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "resources/models/viking_room.obj")) {
		SDL_LogError(0, "Unable to read model %s", err.c_str());
	}

	std::unordered_map<Vertex, uint32_t> unique_vtx{};

	for (const auto &shape : shapes) {
		for (const auto &index : shape.mesh.indices) {
			Vertex vertex{};
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};
			vertex.tex_coord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (unique_vtx.count(vertex) == 0) {
				unique_vtx[vertex] = uint32_t(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(unique_vtx[vertex]);
		}
	}

	{
		vertex_size = sizeof(vertices[0]) * uint32_t(vertices.size());
		Buffer staging_buffer(app, vertex_size, vk::BufferUsageFlagBits::eTransferSrc, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible });
		staging_buffer.write_data(vertices.data(), vertex_size);
		vertex_buffer = std::unique_ptr<Buffer>(new Buffer(app, vertex_size, { vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer }, { vk::MemoryPropertyFlagBits::eDeviceLocal }));
		app.copy_buffer(staging_buffer.buffer, vertex_buffer->buffer, vertex_size);
	}
	{
		index_size = sizeof(indices[0]) * indices.size();
		Buffer staging_buffer(app, index_size, vk::BufferUsageFlagBits::eTransferSrc, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible });
		staging_buffer.write_data(indices.data(), index_size);
		index_buffer = std::unique_ptr<Buffer>(new Buffer(app, index_size, { vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer }, { vk::MemoryPropertyFlagBits::eDeviceLocal }));
		app.copy_buffer(staging_buffer.buffer, index_buffer->buffer, index_size);
	}

	triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat);
	triangles.setVertexData(vertex_buffer->address);
	triangles.setVertexStride(sizeof(Vertex));
	triangles.setIndexType(vk::IndexType::eUint32);
	triangles.setIndexData(index_buffer->address);
	triangles.setTransformData({});
	triangles.setMaxVertex(vertices.size());

	geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
	geometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);
	geometry.geometry.setTriangles(triangles);

	range_info.setPrimitiveCount(indices.size() / 3);
	range_info.setFirstVertex(0);
	range_info.setPrimitiveOffset(0);
	range_info.setTransformOffset(0);
}
void Mesh::draw(VulkanFrame &frame) {
	frame.command_buffer->bindVertexBuffers(0, { vertex_buffer->buffer }, { 0 });
	frame.command_buffer->bindIndexBuffer(index_buffer->buffer, 0, vk::IndexType::eUint32);
	frame.command_buffer->drawIndexed(indices.size(), 1, 0, 0, 0);
}
void Mesh::update(const float delta_time) {
	if (show_window) {
		std::string name = "Mesh" + std::to_string(id);
		if (!ImGui::Begin(name.c_str(), &show_window, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::End();

			return;
		}
		if (ImGui::InputFloat3("Position", &position[0]) ||
				ImGui::InputFloat3("Rotation", &rotation[0]) ||
				ImGui::InputFloat3("Scale", &scale[0])) {
			update_matrix();
		}
		ImGui::End();
	}
}
void Mesh::update_matrix() {
	glm::mat4 rot = glm::mat4_cast(glm::quat(rotation));
	glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
	glm::mat4 size = glm::scale(glm::mat4(1.0f), scale);

	transform = trans * rot * size;
}
int Mesh::get_id() {
	return id_++;
}
