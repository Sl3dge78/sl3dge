//
// Created by Sledge on 2021-01-10.
//

#include "Scene.h"
#include "vulkan/VulkanApplication.h"

uint32_t Scene::load_mesh(const std::string path) {
	meshes.emplace_back(std::make_unique<Mesh>(*app, path));
	return meshes.size() - 1;
}
uint32_t Scene::create_material(const glm::vec3 &albedo, const float roughness, const float metallic, const float ao, const uint32_t albedo_texture) {
	materials.emplace_back(albedo, roughness, metallic, ao, albedo_texture);
	return materials.size() - 1;
}
uint32_t Scene::load_texture(const std::string path) {
	textures.emplace_back(std::make_unique<Texture>(*app, path));
	return textures.size() - 1;
}
MeshInstance *Scene::create_instance(const uint32_t mesh_id, const uint32_t mat_id, glm::mat4 transform) {
	instances.emplace_back(mesh_id, mat_id);
	return &instances.back();
}
void Scene::init() {
	this->app = app;
	build_BLAS({ vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction });
	build_TLAS({ vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace });
	allocate_uniform_buffer();
}
void Scene::allocate_uniform_buffer() {
	{
		instances_size = sizeof(instances[0]) * instances.size();
		scene_desc_buffer = std::unique_ptr<Buffer>(new Buffer(*app, instances_size, { vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer }, { vk::MemoryPropertyFlagBits::eDeviceLocal }));
		refresh_scene_desc();
	}
	{
		materials_size = materials.size() * sizeof(materials[0]);
		materials_buffer = std::unique_ptr<Buffer>(new Buffer(*app, materials_size, { vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer }, { vk::MemoryPropertyFlagBits::eDeviceLocal }));
		refresh_materials();
	}
}
void Scene::refresh_materials() {
	materials_size = materials.size() * sizeof(materials[0]);
	Buffer staging_buffer(*app, materials_size, vk::BufferUsageFlagBits::eTransferSrc, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible });
	staging_buffer.write_data(materials.data(), materials_size);
	app->copy_buffer(staging_buffer.buffer, materials_buffer->buffer, materials_size);
}
void Scene::refresh_scene_desc() {
	instances_size = sizeof(instances[0]) * instances.size();
	Buffer staging_buffer(*app, instances_size, vk::BufferUsageFlagBits::eTransferSrc, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible });
	staging_buffer.write_data(instances.data(), instances_size);
	app->copy_buffer(staging_buffer.buffer, scene_desc_buffer->buffer, instances_size);
}
void Scene::build_BLAS(vk::BuildAccelerationStructureFlagsKHR flags) {
	std::vector<std::unique_ptr<AccelerationStructure>> orig_blas;
	bool do_compaction = (flags & vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction) == vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction;
	auto query_pool = app->get_device().createQueryPoolUnique(vk::QueryPoolCreateInfo({}, vk::QueryType::eAccelerationStructureCompactedSizeKHR, meshes.size(), {}));

	{
		std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> build_infos;
		vk::DeviceSize max_scratch = 0;

		// Create BLAS
		for (auto &mesh : meshes) {
			vk::AccelerationStructureBuildGeometryInfoKHR build_info(
					vk::AccelerationStructureTypeKHR::eBottomLevel,
					flags,
					vk::BuildAccelerationStructureModeKHR::eBuild,
					nullptr, nullptr,
					1, &mesh->geometry, nullptr,
					nullptr);
			auto size_info = app->get_device().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build_info, mesh->get_primitive_count());
			max_scratch = std::max(max_scratch, size_info.buildScratchSize);
			orig_blas.emplace_back(std::make_unique<AccelerationStructure>(*app, vk::AccelerationStructureTypeKHR::eBottomLevel, size_info.accelerationStructureSize));
			build_infos.emplace_back(build_info);
		}

		// Build BLAS
		auto cmd_buffs = app->create_commandbuffers(meshes.size());
		Buffer scratch_buffer(*app, max_scratch, { vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer }, vk::MemoryPropertyFlagBits::eDeviceLocal);
		for (int i = 0; i < meshes.size(); i++) {
			build_infos[i].dstAccelerationStructure = orig_blas[i]->get_acceleration_structure();
			build_infos[i].scratchData.deviceAddress = scratch_buffer.address;
			cmd_buffs[i].buildAccelerationStructuresKHR(build_infos[i], &meshes[i]->range_info);
			cmd_buffs[i].pipelineBarrier(
					vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
					vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {},
					vk::MemoryBarrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR, vk::AccessFlagBits::eAccelerationStructureReadKHR),
					nullptr, nullptr);

			if (do_compaction)
				cmd_buffs[i].writeAccelerationStructuresPropertiesKHR(orig_blas[i]->get_acceleration_structure(), vk::QueryType::eAccelerationStructureCompactedSizeKHR, *query_pool, i);
		}
		app->flush_commandbuffers(cmd_buffs);
	}

	if (do_compaction) {
		// Copy them in the meshes while compressing them
		std::vector<vk::DeviceSize> compact_sizes = app->get_device().getQueryPoolResults<vk::DeviceSize>(*query_pool, 0, meshes.size(), meshes.size() * sizeof(vk::DeviceSize), sizeof(vk::DeviceSize), vk::QueryResultFlagBits::eWait).value;
		auto cmd_buf = app->create_commandbuffer();
		for (int i = 0; i < meshes.size(); i++) {
			meshes[i]->blas = std::make_unique<AccelerationStructure>(*app, vk::AccelerationStructureTypeKHR::eBottomLevel, compact_sizes[i]);
			cmd_buf.copyAccelerationStructureKHR(vk::CopyAccelerationStructureInfoKHR(orig_blas[i]->get_acceleration_structure(), meshes[i]->blas->get_acceleration_structure(), vk::CopyAccelerationStructureModeKHR::eCompact));
		}
		app->flush_commandbuffer(cmd_buf);
	} else {
		// Just move the BLASes into the meshes for future
		for (int i = 0; i < meshes.size(); i++) {
			meshes[i]->blas = std::move(orig_blas[i]);
		}
	}
}
void Scene::build_TLAS(vk::BuildAccelerationStructureFlagsKHR flags, bool update) {
	int hit_goup_id = 0;

	std::vector<vk::AccelerationStructureInstanceKHR> as_instances = {};
	for (int i = 0; i < instances.size(); i++) {
		vk::AccelerationStructureInstanceKHR as_instance = {};
		as_instance.flags = (unsigned int)(vk::GeometryInstanceFlagBitsKHR::eTriangleCullDisable);
		as_instance.instanceShaderBindingTableRecordOffset = hit_goup_id;
		as_instance.instanceCustomIndex = i;
		as_instance.accelerationStructureReference = meshes[instances[i].get_mesh_id()]->blas->get_address();
		as_instance.mask = 0xFF;
		auto transposed = glm::transpose(instances[i].get_transform());
		memcpy(&as_instance, &transposed, sizeof(vk::TransformMatrixKHR));
		as_instances.push_back(as_instance);
	}

	// Update buffers
	if (update) {
		rtx_instances_buffer.reset();
		materials_buffer.reset();
	}
	uint32_t buffer_size = as_instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
	rtx_instances_buffer = std::unique_ptr<Buffer>(new Buffer(*app, buffer_size, { vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR }, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible }));
	rtx_instances_buffer->write_data(as_instances.data(), buffer_size);

	vk::AccelerationStructureGeometryKHR tlasGeo{};
	tlasGeo.geometryType = vk::GeometryTypeKHR::eInstances;
	tlasGeo.flags = vk::GeometryFlagBitsKHR::eOpaque;

	vk::DeviceOrHostAddressConstKHR addr{};
	addr.deviceAddress = rtx_instances_buffer->address;
	vk::AccelerationStructureGeometryInstancesDataKHR geometry_instances_data(false, addr);
	tlasGeo.geometry.instances = geometry_instances_data;

	uint32_t geometry_count = as_instances.size();
	vk::AccelerationStructureBuildGeometryInfoKHR build_info{};
	build_info.flags = flags;
	build_info.geometryCount = 1;
	build_info.pGeometries = &tlasGeo;
	build_info.mode = update ? vk::BuildAccelerationStructureModeKHR::eUpdate : vk::BuildAccelerationStructureModeKHR::eBuild;
	build_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;
	build_info.srcAccelerationStructure = nullptr;

	// Find sizes
	vk::AccelerationStructureBuildSizesInfoKHR size_info{};
	size_info = app->get_device().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build_info, geometry_count);

	if (!update) {
		tlas = std::make_unique<AccelerationStructure>(*app, vk::AccelerationStructureTypeKHR::eTopLevel, size_info.accelerationStructureSize);
	}

	Buffer scratch(*app, size_info.buildScratchSize, { vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress }, { vk::MemoryPropertyFlagBits::eDeviceLocal });
	build_info.srcAccelerationStructure = update ? tlas->get_acceleration_structure() : nullptr;
	build_info.dstAccelerationStructure = tlas->get_acceleration_structure();
	build_info.scratchData.deviceAddress = scratch.address;

	vk::AccelerationStructureBuildRangeInfoKHR build_range(geometry_count, 0, 0, 0);

	auto cmd_buf = app->create_commandbuffer();
	cmd_buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, {}, vk::MemoryBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eAccelerationStructureWriteKHR), nullptr, nullptr);
	cmd_buf.buildAccelerationStructuresKHR(build_info, &build_range);
	app->flush_commandbuffer(cmd_buf);

	debug_name_object(app->get_device(), uint64_t(VkAccelerationStructureKHR(tlas->get_acceleration_structure())), vk::ObjectType::eAccelerationStructureKHR, "Scene TLAS");
}
void Scene::draw(vk::CommandBuffer cmd) {
	for (uint32_t i = 0; i < instances.size(); i++) {
		auto mesh = meshes[instances[i].get_mesh_id()].get();
		mesh->draw(cmd, i);
	}
}
void Scene::draw_menu_bar() {
	if (ImGui::BeginMenu("Materials")) {
		for (auto &material : materials) {
			ImGui::MenuItem("Object", "", &material.draw_ui);
		}
		ImGui::EndMenu();
	}
	bool changed = false;
	int id = 0;
	for (auto &material : materials) {
		ImGui::PushID(id);
		changed |= material.on_gui();
		ImGui::PopID();
		id++;
	}
	if (changed) {
		refresh_materials();
	}
}
