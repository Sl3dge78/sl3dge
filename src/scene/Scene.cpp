//
// Created by Sledge on 2021-01-10.
//

#include "Scene.h"
#include "vulkan/VulkanApplication.h"

Mesh *Scene::load_mesh(const std::string path) {
	return meshes.emplace_back(std::make_unique<Mesh>(*app, path, uint32_t(meshes.size()))).get();
}

Material *Scene::create_material(const glm::vec3 &albedo, const float roughness, const float metallic, const float ao, const uint32_t albedo_texture) {
	return materials.emplace_back(std::make_unique<Material>(uint32_t(materials.size()), albedo, roughness, metallic, ao, albedo_texture)).get();
}

uint32_t Scene::load_texture(const std::string path) {
	textures.emplace_back(std::make_unique<Texture>(*app, path));
	return textures.size() - 1;
}
/*
MeshInstance_ *Scene::create_instance(const uint32_t mesh_id, const uint32_t mat_id, glm::mat4 transform) {
	instances.emplace_back(mesh_id, mat_id);
	return &instances.back();
}
*/
uint32_t Scene::create_light(const int32_t type, const glm::vec3 color, const float intensity, const glm::vec3 vec, const bool cast_shadows) {
	Light light{
		.type = type, // TODO : use enum
		.color = color,
		.intensity = intensity,
		.vec = vec,
		.cast_shadows = cast_shadows
	};

	lights.emplace_back(light);
	return lights.size() - 1;
}
void Scene::init() {
	this->app = app;
	build_BLAS({ vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction });
	build_TLAS({ vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace });
	update_buffers();

	texture_sampler = app->get_device().createSamplerUnique(vk::SamplerCreateInfo(
			{}, vk::Filter::eLinear, vk::Filter::eLinear,
			vk::SamplerMipmapMode::eNearest,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			0,
			true, app->get_physical_device().getProperties().limits.maxSamplerAnisotropy,
			false, vk::CompareOp::eAlways, 0, 0,
			vk::BorderColor::eIntOpaqueBlack, false));
}
void Scene::write_descriptors(VulkanPipeline &pipeline) {
	uint32_t mesh_count = meshes.size();
	uint32_t texture_count = textures.size();

	vk::DescriptorSetLayoutBinding binding(1, vk::DescriptorType::eAccelerationStructureKHR, 1, { vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eFragment }, nullptr);
	vk::DescriptorPoolSize pool_size(vk::DescriptorType::eAccelerationStructureKHR, 1);
	std::vector<vk::AccelerationStructureKHR> as{ get_tlas() };
	vk::WriteDescriptorSetAccelerationStructureKHR as_desc(as);
	vk::WriteDescriptorSet as_descriptor_write(nullptr, 1, 0, 1, vk::DescriptorType::eAccelerationStructureKHR, nullptr, nullptr, nullptr);
	as_descriptor_write.pNext = &as_desc;
	pipeline.add_descriptor(binding, pool_size, as_descriptor_write);

	vk::DescriptorBufferInfo bi_camera_matrices(camera.buffer->buffer, 0, VK_WHOLE_SIZE);
	pipeline.add_descriptor(vk::DescriptorType::eUniformBuffer, bi_camera_matrices, 2, { vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment });

	vk::DescriptorBufferInfo bi_scene(scene_desc_buffer->buffer, 0, VK_WHOLE_SIZE);
	pipeline.add_descriptor(vk::DescriptorType::eStorageBuffer, bi_scene, 3, { vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment });

	std::vector<vk::DescriptorBufferInfo> bi_vtx;
	std::vector<vk::DescriptorBufferInfo> bi_idx;
	for (auto &mesh : meshes) {
		bi_vtx.emplace_back(mesh->get_vtx_buffer(), 0, VK_WHOLE_SIZE);
		bi_idx.emplace_back(mesh->get_idx_buffer(), 0, VK_WHOLE_SIZE);
	}

	pipeline.add_descriptor(vk::DescriptorType::eStorageBuffer, bi_vtx, 4, { vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eFragment }, 1, mesh_count);
	pipeline.add_descriptor(vk::DescriptorType::eStorageBuffer, bi_idx, 5, { vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eFragment }, 1, mesh_count);

	std::vector<vk::DescriptorImageInfo> images_info;
	for (auto &tex : textures) {
		images_info.emplace_back(*texture_sampler, tex->texture->image_view, vk::ImageLayout::eShaderReadOnlyOptimal);
	}
	pipeline.add_descriptor(vk::DescriptorType::eCombinedImageSampler, images_info, 6, { vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eFragment }, 1, texture_count);

	// Binding 7 ; Materials
	vk::DescriptorBufferInfo material_info(materials_buffer->buffer, 0, VK_WHOLE_SIZE);
	pipeline.add_descriptor(vk::DescriptorType::eStorageBuffer, material_info, 7, { vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eFragment });

	vk::PushConstantRange push_constant{ vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstant) };
	pipeline.add_push_constant(push_constant);
	pipeline.build_descriptors(app->get_device());
}
void Scene::update(float delta_time) {
	camera.update(delta_time);
}
void Scene::update_buffers() {
	{
		struct Instance {
			alignas(4) uint32_t mesh_id;
			alignas(4) uint32_t mat_id;
			alignas(16) glm::mat4 transform;
			alignas(16) glm::mat4 inverted;
		};

		std::vector<Instance> instances;
		for (int i = 0; i < nodes.size(); i++) {
			MeshInstance *mi = dynamic_cast<MeshInstance *>(nodes[i].get());
			if (mi == nullptr)
				continue;

			// TODO : Le contenu de material->id change sans que je comprenne pourquoi

			instances.emplace_back(Instance{
					.mesh_id = mi->get_mesh_id(),
					.mat_id = mi->get_material_id(),
					.transform = mi->get_transform(),
					.inverted = glm::inverse(mi->get_transform()) });
		}

		vk::DeviceSize instances_size = sizeof(Instance) * instances.size();
		scene_desc_buffer = std::unique_ptr<Buffer>(new Buffer(*app, instances_size, { vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer }, { vk::MemoryPropertyFlagBits::eDeviceLocal }));
		Buffer staging_buffer(*app, instances_size, vk::BufferUsageFlagBits::eTransferSrc, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible });
		staging_buffer.write_data(instances.data(), instances_size);
		app->copy_buffer(staging_buffer.buffer, scene_desc_buffer->buffer, instances_size);
	}
	{
		struct Mat {
			alignas(16) glm::vec3 albedo;
			alignas(4) int32_t albedo_texture_id;
			alignas(4) float roughness;
			alignas(4) float metallic;
			alignas(4) float ao;
			alignas(4) float rim_pow;
			alignas(4) float rim_strength;
		};

		std::vector<Mat> mats;
		for (auto &mat : materials) {
			mats.emplace_back(Mat{
					.albedo = mat->albedo,
					.albedo_texture_id = mat->albedo_texture_id,
					.roughness = mat->roughness,
					.metallic = mat->metallic,
					.ao = mat->ao,
					.rim_pow = mat->rim_pow,
					.rim_strength = mat->rim_strength });
		}

		vk::DeviceSize materials_size = mats.size() * sizeof(Mat);
		materials_buffer = std::unique_ptr<Buffer>(new Buffer(*app, materials_size, { vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer }, { vk::MemoryPropertyFlagBits::eDeviceLocal }));
		Buffer staging_buffer(*app, materials_size, vk::BufferUsageFlagBits::eTransferSrc, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible });
		staging_buffer.write_data(mats.data(), materials_size);
		app->copy_buffer(staging_buffer.buffer, materials_buffer->buffer, materials_size);
	}
	{
		vk::DeviceSize lights_size = lights.size() * sizeof(lights[0]);
		lights_buffer = std::unique_ptr<Buffer>(new Buffer(*app, lights_size, { vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer }, { vk::MemoryPropertyFlagBits::eDeviceLocal }));
		Buffer staging_buffer(*app, lights_size, vk::BufferUsageFlagBits::eTransferSrc, { vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible });
		staging_buffer.write_data(lights.data(), lights_size);
		app->copy_buffer(staging_buffer.buffer, lights_buffer->buffer, lights_size);
	}
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
	std::vector<vk::AccelerationStructureInstanceKHR> as_instances = {};
	for (int i = 0; i < nodes.size(); i++) {
		MeshInstance *mi = dynamic_cast<MeshInstance *>(nodes[i].get());
		if (mi == nullptr)
			continue;

		as_instances.push_back(mi->get_ASInstance_data());
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
	for (int i = 0; i < nodes.size(); i++) {
		VisualInstance *vi = dynamic_cast<VisualInstance *>(nodes[i].get());
		if (vi == nullptr)
			continue;
		vi->draw(cmd);
	}
}
/*
void Scene::draw_menu_bar() {
	// TODO : clean that up
	{
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
	{
		bool changed = false;

		if (ImGui::BeginMenu("Lights")) {
			ImGui::MenuItem("Lights", "", &draw_lights_ui);

			ImGui::EndMenu();
		}
		if (draw_lights_ui) {
			ImGui::Begin("Light");
			int id = 0;
			for (auto &light : lights) {
				ImGui::PushID(id);
				changed |= ImGui::InputInt("Type", &light.type);
				changed |= ImGui::ColorEdit3("Color", &light.color.r);
				changed |= ImGui::DragFloat("Intensity", &light.intensity, 1.0f, 0.0f, 100000.0f);
				changed |= ImGui::InputFloat3("Vector", &light.vec.x, 0.01f);
				changed |= ImGui::InputInt("Cast shadows", &light.cast_shadows);
				ImGui::Spacing();
				ImGui::PopID();
				id++;
			}
			ImGui::End();
		}

		if (changed) {
			refresh_lights();
		}
	}
}
*/
