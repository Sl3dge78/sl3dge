//
// Created by Sledge on 2021-01-03.
//

#include "Mesh.h"
#include "VulkanApplication.h"

Mesh::Mesh(const std::string path) {
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
}
void Mesh::load(VulkanApplication *app) {
	{
		vk::DeviceSize vertex_size = sizeof(vertices[0]) * uint32_t(vertices.size());
		Buffer staging_buffer(app->get_device(), app->get_physical_device(), vertex_size, vk::BufferUsageFlagBits::eTransferSrc, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible });
		staging_buffer.write_data(vertices.data(), vertex_size);
		vertex_buffer = std::unique_ptr<Buffer>(new Buffer(app->get_device(), app->get_physical_device(), vertex_size, { vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer }, { vk::MemoryPropertyFlagBits::eDeviceLocal }));
		app->copy_buffer(staging_buffer.buffer, vertex_buffer->buffer, vertex_size);
	}
	{
		vk::DeviceSize index_size = sizeof(indices[0]) * indices.size();
		Buffer staging_buffer(app->get_device(), app->get_physical_device(), index_size, vk::BufferUsageFlagBits::eTransferSrc, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible });
		staging_buffer.write_data(indices.data(), index_size);
		index_buffer = std::unique_ptr<Buffer>(new Buffer(app->get_device(), app->get_physical_device(), index_size, { vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer }, { vk::MemoryPropertyFlagBits::eDeviceLocal }));
		app->copy_buffer(staging_buffer.buffer, index_buffer->buffer, index_size);
	}

	vk::AccelerationStructureGeometryTrianglesDataKHR triangles;
	triangles.setVertexFormat(vk::Format::eR32G32B32A32Sfloat);
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
void Mesh::draw(vk::CommandBuffer cmd_buf) {
	cmd_buf.bindVertexBuffers(0, { vertex_buffer->buffer }, { 0 });
	cmd_buf.bindIndexBuffer(index_buffer->buffer,0, vk::IndexType::eUint32);
	cmd_buf.drawIndexed(indices.size(), 1, 0, 0, 0);
}

