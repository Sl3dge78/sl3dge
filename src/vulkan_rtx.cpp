#include <vulkan/vulkan.h>
#include <sl3dge/types.h>
#include "vulkan_layer.h"

/*
 === TODO ===
 CRITICAL
 - Allow creation of blas in series (input a cmd buffer)

 MAJOR
- If we have multiple meshes that use the same primitve, we will just dupe it
here

BACKLOG


 IMPROVEMENTS

*/

typedef struct VulkanRTXRenderer {
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkDescriptorPool descriptor_pool;

	VkDescriptorSet descriptor_sets[2];
	u32 descriptor_set_count;

	VkDescriptorSetLayout app_set_layout;
	VkDescriptorSetLayout game_set_layout;

	u32 push_constant_size;

	Image render_image;

	Buffer rgen_sbt;
	Buffer rmiss_sbt;
	Buffer rchit_sbt;

	VkStridedDeviceAddressRegionKHR strides[4];

	Buffer BLAS_buffer;
	VkAccelerationStructureKHR BLAS;
	Buffer TLAS_buffer;
	VkAccelerationStructureKHR TLAS;
	Buffer instance_data;

	Buffer gltf_buffer;
	Buffer vertex_buffer;
	Buffer index_buffer;

} VulkanRTXRenderer;

/*
	LOAD_DEVICE_FUNC(vkCreateRayTracingPipelinesKHR);
	LOAD_DEVICE_FUNC(vkCmdTraceRaysKHR);
	LOAD_DEVICE_FUNC(vkGetRayTracingShaderGroupHandlesKHR);
	LOAD_DEVICE_FUNC(vkGetBufferDeviceAddressKHR);
	LOAD_DEVICE_FUNC(vkCreateAccelerationStructureKHR);
	LOAD_DEVICE_FUNC(vkGetAccelerationStructureBuildSizesKHR);
	LOAD_DEVICE_FUNC(vkCmdBuildAccelerationStructuresKHR);
	LOAD_DEVICE_FUNC(vkDestroyAccelerationStructureKHR);
	LOAD_DEVICE_FUNC(vkGetAccelerationStructureDeviceAddressKHR);

const char* extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		VK_KHR_RAY_QUERY_EXTENSION_NAME,
		VK_KHR_SHADER_CLOCK_EXTENSION_NAME

};

// Features
	VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features =
{}; descriptor_indexing_features.sType =
VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	descriptor_indexing_features.pNext = NULL;
	descriptor_indexing_features.shaderStorageBufferArrayNonUniformIndexing =
VK_TRUE; descriptor_indexing_features.runtimeDescriptorArray = VK_TRUE;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_feature = {};
	accel_feature.sType =
VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accel_feature.pNext = &descriptor_indexing_features;
	accel_feature.accelerationStructure = VK_TRUE;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_feature = {};
	rt_feature.sType =
VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	rt_feature.pNext = &accel_feature;
	rt_feature.rayTracingPipeline = VK_TRUE;

	VkPhysicalDeviceBufferDeviceAddressFeatures device_address = {};
	device_address.sType =
VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	device_address.pNext = &rt_feature;
	device_address.bufferDeviceAddress = VK_TRUE;


*/

internal void CreateRtxSbt(VulkanContext *context, VkPipeline pipeline, VulkanShaderBindingTable *sbt) {
	{
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtx_properties = {};
		rtx_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

		VkPhysicalDeviceProperties2 properties = {};
		properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		properties.pNext = &rtx_properties;

		vkGetPhysicalDeviceProperties2(context->physical_device, &properties);

		const u32 handle_size = rtx_properties.shaderGroupHandleSize;
		const u32 handle_size_aligned = aligned_size(rtx_properties.shaderGroupHandleSize, rtx_properties.shaderGroupHandleAlignment);
		const u32 group_count = 4;
		const u32 sbt_size = group_count * handle_size_aligned;

		CreateBuffer(context->device, &context->memory_properties, handle_size_aligned,
				VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &sbt->rgen);
		DEBUGNameBuffer(context->device, &sbt->rgen, "RGEN SBT");

		CreateBuffer(context->device, &context->memory_properties, handle_size_aligned,
				VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &sbt->rmiss);
		DEBUGNameBuffer(context->device, &sbt->rmiss, "RMISS SBT");

		CreateBuffer(context->device, &context->memory_properties, handle_size_aligned,
				VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &sbt->rchit);
		DEBUGNameBuffer(context->device, &sbt->rchit, "RCHIT SBT");

		u8 *data = (u8 *)malloc(sbt_size);

		VkResult result = pfn_vkGetRayTracingShaderGroupHandlesKHR(context->device, pipeline, 0, group_count, sbt_size, data);
		AssertVkResult(result);

		UploadToBuffer(context->device, &sbt->rgen, data, handle_size_aligned);
		UploadToBuffer(context->device, &sbt->rmiss, data + handle_size_aligned, handle_size_aligned * 2);
		UploadToBuffer(context->device, &sbt->rchit, data + (3 * handle_size_aligned), handle_size_aligned);

		sbt->strides[0] = { sbt->rgen.address, handle_size_aligned, handle_size_aligned };
		sbt->strides[1] = { sbt->rmiss.address, handle_size_aligned, handle_size_aligned };
		sbt->strides[2] = { sbt->rchit.address, handle_size_aligned, handle_size_aligned };
		sbt->strides[3] = { 0u, 0u, 0u };
		free(data);
	}
}

internal void CreateRtxDescriptorSet(VulkanContext *context, const VkAccelerationStructureKHR *TLAS, VkBuffer vtx_buffer, VkBuffer idx_buffer,
		VkDescriptorSetLayout *set_layout, VkDescriptorSet *descriptor_set) {
	const VkDescriptorSetLayoutBinding bindings[] = {
		// FINAL IMAGE
		{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, NULL },
		// ACCELERATION STRUCTURE
		{ 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,
				VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, NULL },
		// VERTEX BUFFER
		{ 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, NULL },
		// INDEX BUFFER
		{ 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, NULL },
	};

	const u32 descriptor_count = sizeof(bindings) / sizeof(bindings[0]);

	VkDescriptorSetLayoutCreateInfo game_set_create_info = {};
	game_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	game_set_create_info.pNext = NULL;
	game_set_create_info.flags = 0;
	game_set_create_info.bindingCount = descriptor_count;
	game_set_create_info.pBindings = bindings;
	AssertVkResult(vkCreateDescriptorSetLayout(context->device, &game_set_create_info, NULL, set_layout));

	VkDescriptorSetAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.descriptorPool = context->descriptor_pool;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = set_layout;
	AssertVkResult(vkAllocateDescriptorSets(context->device, &allocate_info, descriptor_set));

	// Writes
	const u32 static_writes_count = 4;
	VkWriteDescriptorSet static_writes[static_writes_count];

	VkDescriptorImageInfo img_info = { VK_NULL_HANDLE, context->rtx_image.image_view, VK_IMAGE_LAYOUT_GENERAL };
	static_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	static_writes[0].pNext = NULL;
	static_writes[0].dstSet = *descriptor_set;
	static_writes[0].dstBinding = 0;
	static_writes[0].dstArrayElement = 0;
	static_writes[0].descriptorCount = 1;
	static_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	static_writes[0].pImageInfo = &img_info;
	static_writes[0].pBufferInfo = NULL;
	static_writes[0].pTexelBufferView = NULL;

	VkWriteDescriptorSetAccelerationStructureKHR as_info = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, NULL, 1, TLAS };
	static_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	static_writes[1].pNext = &as_info;
	static_writes[1].dstSet = *descriptor_set;
	static_writes[1].dstBinding = 1;
	static_writes[1].dstArrayElement = 0;
	static_writes[1].descriptorCount = 1;
	static_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	static_writes[1].pImageInfo = NULL;
	static_writes[1].pBufferInfo = NULL;
	static_writes[1].pTexelBufferView = NULL;

	VkDescriptorBufferInfo vtx_info = { vtx_buffer, 0, VK_WHOLE_SIZE };
	static_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	static_writes[2].pNext = NULL;
	static_writes[2].dstSet = *descriptor_set;
	static_writes[2].dstBinding = 2;
	static_writes[2].dstArrayElement = 0;
	static_writes[2].descriptorCount = 1;
	static_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	static_writes[2].pImageInfo = NULL;
	static_writes[2].pBufferInfo = &vtx_info;
	static_writes[2].pTexelBufferView = NULL;

	VkDescriptorBufferInfo idx_info = { idx_buffer, 0, VK_WHOLE_SIZE };
	static_writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	static_writes[3].pNext = NULL;
	static_writes[3].dstSet = *descriptor_set;
	static_writes[3].dstBinding = 2;
	static_writes[3].dstArrayElement = 0;
	static_writes[3].descriptorCount = 1;
	static_writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	static_writes[3].pImageInfo = NULL;
	static_writes[3].pBufferInfo = &idx_info;
	static_writes[3].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(context->device, static_writes_count, static_writes, 0, 0);
}

internal void CreateRtxRenderImage(
		const VkDevice device, const Swapchain *swapchain, const VkPhysicalDeviceMemoryProperties *memory_properties, Image *image) {
	VkImageCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.imageType = VK_IMAGE_TYPE_2D;
	create_info.format = swapchain->format;
	create_info.extent = { swapchain->extent.width, swapchain->extent.height, 1 };
	create_info.mipLevels = 1;
	create_info.arrayLayers = 1;
	create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = NULL;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkResult result = vkCreateImage(device, &create_info, NULL, &image->image);
	AssertVkResult(result);

	VkMemoryRequirements requirements = {};
	vkGetImageMemoryRequirements(device, image->image, &requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.allocationSize = requirements.size;
	alloc_info.memoryTypeIndex = FindMemoryType(memory_properties, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	result = vkAllocateMemory(device, &alloc_info, NULL, &image->memory);
	AssertVkResult(result);

	result = vkBindImageMemory(device, image->image, image->memory, 0);
	AssertVkResult(result);

	VkImageViewCreateInfo imv_ci = {};
	imv_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imv_ci.pNext = NULL;
	imv_ci.flags = 0;
	imv_ci.image = image->image;
	imv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imv_ci.format = swapchain->format;
	imv_ci.components = {};
	imv_ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	result = vkCreateImageView(device, &imv_ci, NULL, &image->image_view);
	AssertVkResult(result);
	DEBUGNameImage(device, image, "RTX Render Image");
}

internal void CreateRtxPipeline(const VkDevice device, const VkPipelineLayout *layout, VkPipeline *pipeline) {
	VkRayTracingPipelineCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	create_info.pNext = NULL;
	create_info.flags = 0;

	// SHADERS
	const u32 shader_count = 4;
	create_info.stageCount = shader_count;
	VkPipelineShaderStageCreateInfo temp = {};
	temp.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	temp.pNext = NULL;
	temp.flags = 0;
	temp.pName = "main";
	temp.pSpecializationInfo = NULL;

	VkPipelineShaderStageCreateInfo rgen = temp;
	rgen.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	CreateVkShaderModule("resources/shaders/raytrace.rgen.spv", device, &rgen.module);

	VkPipelineShaderStageCreateInfo rmiss = temp;
	rmiss.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	CreateVkShaderModule("resources/shaders/raytrace.rmiss.spv", device, &rmiss.module);

	VkPipelineShaderStageCreateInfo rmiss_shadow = temp;
	rmiss_shadow.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	CreateVkShaderModule("resources/shaders/shadow.rmiss.spv", device, &rmiss_shadow.module);

	VkPipelineShaderStageCreateInfo rchit = temp;
	rchit.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	CreateVkShaderModule("resources/shaders/raytrace.rchit.spv", device, &rchit.module);

	VkPipelineShaderStageCreateInfo stages[shader_count] = { rgen, rmiss, rmiss_shadow, rchit };
	create_info.pStages = stages;

	// GROUPS
	create_info.groupCount = shader_count;
	VkRayTracingShaderGroupCreateInfoKHR group_temp = {}; // Used as template
	group_temp.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	group_temp.pNext = NULL;
	group_temp.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	group_temp.generalShader = VK_SHADER_UNUSED_KHR;
	group_temp.closestHitShader = VK_SHADER_UNUSED_KHR;
	group_temp.anyHitShader = VK_SHADER_UNUSED_KHR;
	group_temp.intersectionShader = VK_SHADER_UNUSED_KHR;
	group_temp.pShaderGroupCaptureReplayHandle = NULL;

	VkRayTracingShaderGroupCreateInfoKHR rgen_group = group_temp;
	rgen_group.generalShader = 0;
	VkRayTracingShaderGroupCreateInfoKHR rmiss_group = group_temp;
	rmiss_group.generalShader = 1;
	VkRayTracingShaderGroupCreateInfoKHR rmiss_shadow_group = group_temp;
	rmiss_shadow_group.generalShader = 2;
	VkRayTracingShaderGroupCreateInfoKHR rchit_group = group_temp;
	rchit_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	rchit_group.closestHitShader = 3;
	VkRayTracingShaderGroupCreateInfoKHR groups[shader_count] = { rgen_group, rmiss_group, rmiss_shadow_group, rchit_group };
	create_info.pGroups = groups;

	// create_info.maxPipelineRayRecursionDepth =
	// rtx_properties->maxRayRecursionDepth;
	create_info.maxPipelineRayRecursionDepth = 0;
	create_info.pLibraryInfo = NULL;
	create_info.pLibraryInterface = NULL;
	create_info.pDynamicState = NULL;
	create_info.layout = *layout;
	create_info.basePipelineHandle = NULL;
	create_info.basePipelineIndex = 0;

	VkResult result = pfn_vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &create_info, NULL, pipeline);
	AssertVkResult(result);

	vkDestroyShaderModule(device, rgen.module, NULL);
	vkDestroyShaderModule(device, rmiss.module, NULL);
	vkDestroyShaderModule(device, rmiss_shadow.module, NULL);
	vkDestroyShaderModule(device, rchit.module, NULL);
}

// =========================
//
// NOTE(Guigui): ACCELERATION STRUCTURES
//
// =========================

internal void CreateBLAS(VulkanContext *context, VkDeviceAddress vtx_address, const u32 vtx_count, VkDeviceAddress idx_address,
		const u32 triangle_count, Buffer *buffer, VkAccelerationStructureKHR *BLAS) {
	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.pNext = NULL;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	geometry.geometry.triangles.pNext = NULL;
	geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	geometry.geometry.triangles.vertexData.deviceAddress = vtx_address;
	geometry.geometry.triangles.vertexStride = sizeof(Vertex);
	geometry.geometry.triangles.maxVertex = vtx_count;
	geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.indexData.deviceAddress = idx_address;
	geometry.geometry.triangles.transformData.deviceAddress = VK_NULL_HANDLE;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.pNext = NULL;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.srcAccelerationStructure = VK_NULL_HANDLE;
	build_info.geometryCount = 1;
	build_info.pGeometries = &geometry;
	build_info.ppGeometries = NULL;
	build_info.scratchData.deviceAddress = NULL;

	// Query the build size
	VkAccelerationStructureBuildSizesInfoKHR size_info = {};
	size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	pfn_vkGetAccelerationStructureBuildSizesKHR(
			context->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, &triangle_count, &size_info);

	// Alloc the scratch buffer
	Buffer scratch;
	CreateBuffer(context->device, &context->memory_properties, size_info.buildScratchSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &scratch);
	build_info.scratchData.deviceAddress = scratch.address;

	// Alloc the blas buffer
	CreateBuffer(context->device, &context->memory_properties, size_info.accelerationStructureSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer);
	DEBUGNameBuffer(context->device, buffer, "BLAS");

	VkAccelerationStructureCreateInfoKHR blas_ci = {};
	blas_ci.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	blas_ci.pNext = NULL;
	blas_ci.createFlags = 0;
	blas_ci.buffer = buffer->buffer;
	blas_ci.offset = 0;
	blas_ci.size = size_info.accelerationStructureSize; // Will be queried below
	blas_ci.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	blas_ci.deviceAddress = 0;
	VkResult result = pfn_vkCreateAccelerationStructureKHR(context->device, &blas_ci, NULL, BLAS);
	AssertVkResult(result);
	DEBUGNameObject(context->device, (u64)*BLAS, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, "BLAS");

	// Now build it !!
	build_info.dstAccelerationStructure = *BLAS;
	VkAccelerationStructureBuildRangeInfoKHR range_info = { triangle_count, 0, 0, 0 };
	VkAccelerationStructureBuildRangeInfoKHR *infos[] = { &range_info };

	VkCommandBuffer cmd_buffer;
	AllocateCommandBuffers(context->device, context->graphics_command_pool, 1, &cmd_buffer);
	BeginCommandBuffer(context->device, cmd_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	pfn_vkCmdBuildAccelerationStructuresKHR(cmd_buffer, 1, &build_info, infos);
	VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, NULL, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR };

	vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			0, 1, &barrier, 0, NULL, 0, NULL);
	vkEndCommandBuffer(cmd_buffer);
	VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmd_buffer, 0, NULL };
	AssertVkResult(vkQueueSubmit(context->graphics_queue, 1, &si, VK_NULL_HANDLE));
	AssertVkResult(vkQueueWaitIdle(context->graphics_queue));

	vkFreeCommandBuffers(context->device, context->graphics_command_pool, 1, &cmd_buffer);
	DestroyBuffer(context->device, &scratch);
}

internal void CreateInstanceGeometry(
		VulkanContext *context, Mat4 mat, VkAccelerationStructureKHR BLAS, Buffer *instance_data, VkAccelerationStructureGeometryKHR *geometry) {
	mat4_transpose(&mat);
	VkTransformMatrixKHR xform = {};
	memcpy(&xform, &mat, sizeof(xform));

	VkAccelerationStructureDeviceAddressInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	info.pNext = NULL;
	info.accelerationStructure = BLAS;

	VkAccelerationStructureInstanceKHR instance = {};
	instance.transform = xform;
	instance.instanceCustomIndex = 0;
	instance.mask = 0xFF;
	instance.instanceShaderBindingTableRecordOffset = 0;
	instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	instance.accelerationStructureReference = (u64)pfn_vkGetAccelerationStructureDeviceAddressKHR(context->device, &info);

	CreateBuffer(context->device, &context->memory_properties, sizeof(VkAccelerationStructureInstanceKHR),
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, instance_data);
	DEBUGNameBuffer(context->device, instance_data, "Instance Data");
	UploadToBuffer(context->device, instance_data, &instance, sizeof(VkAccelerationStructureInstanceKHR));
	// TODO : don't create 1 buffer for each instance

	geometry->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry->pNext = NULL;
	geometry->geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry->geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry->geometry.instances.pNext = NULL;
	geometry->geometry.instances.arrayOfPointers = VK_FALSE;
	geometry->geometry.instances.data.deviceAddress = instance_data->address;
	geometry->flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
}

internal void CreateTLAS(VulkanContext *context, const u32 primitive_count, VkAccelerationStructureGeometryKHR *geometries, Buffer *TLAS_buffer,
		VkAccelerationStructureKHR *TLAS) {
	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.pNext = NULL;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.srcAccelerationStructure = VK_NULL_HANDLE;
	build_info.dstAccelerationStructure = VK_NULL_HANDLE; // Set below
	build_info.geometryCount = primitive_count;
	build_info.pGeometries = geometries;
	build_info.ppGeometries = NULL;
	build_info.scratchData.deviceAddress = NULL; // Set below

	// Get size info
	VkAccelerationStructureBuildSizesInfoKHR size_info = {};
	size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	pfn_vkGetAccelerationStructureBuildSizesKHR(
			context->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, &primitive_count, &size_info);
	CreateBuffer(context->device, &context->memory_properties, size_info.accelerationStructureSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, TLAS_buffer);
	DEBUGNameBuffer(context->device, TLAS_buffer, "TLAS");
	Buffer scratch;
	CreateBuffer(context->device, &context->memory_properties, size_info.buildScratchSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &scratch);

	VkAccelerationStructureCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	create_info.pNext = NULL;
	create_info.createFlags = 0;
	create_info.buffer = TLAS_buffer->buffer;
	create_info.offset = 0;
	create_info.size = size_info.accelerationStructureSize;
	create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	create_info.deviceAddress = 0;
	AssertVkResult(pfn_vkCreateAccelerationStructureKHR(context->device, &create_info, NULL, TLAS));
	DEBUGNameObject(context->device, (u64)*TLAS, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, "TLAS");

	// Now build it
	build_info.dstAccelerationStructure = *TLAS;
	build_info.scratchData.deviceAddress = scratch.address;

	VkAccelerationStructureBuildRangeInfoKHR range_info = { primitive_count, 0, 0, 0 };
	VkAccelerationStructureBuildRangeInfoKHR *infos[] = { &range_info };

	vkDeviceWaitIdle(context->device);

	VkCommandBuffer cmd_buffer;
	AllocateCommandBuffers(context->device, context->graphics_command_pool, 1, &cmd_buffer);
	BeginCommandBuffer(context->device, cmd_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, NULL, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR };

	vkCmdPipelineBarrier(
			cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, NULL, 0, NULL);

	pfn_vkCmdBuildAccelerationStructuresKHR(cmd_buffer, 1, &build_info, infos);
	vkEndCommandBuffer(cmd_buffer);

	VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmd_buffer, 0, NULL };
	AssertVkResult(vkQueueSubmit(context->graphics_queue, 1, &si, VK_NULL_HANDLE));
	AssertVkResult(vkQueueWaitIdle(context->graphics_queue));

	vkFreeCommandBuffers(context->device, context->graphics_command_pool, 1, &cmd_buffer);
	DestroyBuffer(context->device, &scratch);
}

DLL_EXPORT void VulkanDrawRTXFrame(VulkanContext *context, Scene *scene) {
	u32 image_id;
	Swapchain *swapchain = &context->swapchain;

	VkResult result;
	result = vkAcquireNextImageKHR(context->device, swapchain->swapchain, UINT64_MAX, swapchain->image_acquired_semaphore[swapchain->semaphore_id],
			VK_NULL_HANDLE, &image_id);
	AssertVkResult(result); // TODO(Guigui): Recreate swapchain if Suboptimal or
							// outofdate;

	// If the frame hasn't finished rendering wait for it to finish
	result = vkWaitForFences(context->device, 1, &swapchain->fences[image_id], VK_TRUE, UINT64_MAX);
	AssertVkResult(result);
	vkResetFences(context->device, 1, &swapchain->fences[image_id]);

	VkCommandBuffer cmd = swapchain->command_buffers[image_id];

	const VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL };
	result = vkBeginCommandBuffer(cmd, &begin_info);
	AssertVkResult(result);

	VkImageMemoryBarrier img_barrier = {};
	img_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	img_barrier.pNext = NULL;
	img_barrier.srcAccessMask = 0;
	img_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	img_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	img_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	img_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	img_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	img_barrier.image = context->rtx_image.image;
	img_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &img_barrier);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, scene->pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, scene->layout, 0, scene->descriptor_set_count, scene->descriptor_sets, 0, 0);
	// vkCmdPushConstants(cmd, pipeline->layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
	// 0,pipeline->push_constant_size, &context->push_constants);

	pfn_vkCmdTraceRaysKHR(cmd, &scene->sbt.strides[0], &scene->sbt.strides[1], &scene->sbt.strides[2], &scene->sbt.strides[3],
			swapchain->extent.width, swapchain->extent.height, 1);

	img_barrier.image = context->rtx_image.image;
	img_barrier.srcAccessMask = 0;
	img_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	img_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	img_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &img_barrier);

	img_barrier.image = swapchain->images[image_id];
	img_barrier.srcAccessMask = 0;
	img_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	img_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	img_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &img_barrier);

	VkImageCopy image_copy = {};
	image_copy.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	image_copy.srcOffset = { 0, 0, 0 };
	image_copy.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	image_copy.dstOffset = { 0, 0, 0 };
	image_copy.extent = { swapchain->extent.width, swapchain->extent.height, 1 };
	vkCmdCopyImage(cmd, context->rtx_image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain->images[image_id],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

	img_barrier.image = context->rtx_image.image;
	img_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	img_barrier.dstAccessMask = 0;
	img_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	img_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &img_barrier);

	img_barrier.image = swapchain->images[image_id];
	img_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	img_barrier.dstAccessMask = 0;
	img_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	img_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &img_barrier);

	result = vkEndCommandBuffer(cmd);
	AssertVkResult(result);

	// Submit the queue
	// TODO(Guigui): Record buffers beforehand, no need to do it each frame now
	const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &swapchain->image_acquired_semaphore[swapchain->semaphore_id];
	submit_info.pWaitDstStageMask = &stage;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &swapchain->render_complete_semaphore[swapchain->semaphore_id];
	result = vkQueueSubmit(context->graphics_queue, 1, &submit_info, swapchain->fences[image_id]);
	AssertVkResult(result);

	// Present
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &swapchain->render_complete_semaphore[swapchain->semaphore_id];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain->swapchain;
	present_info.pImageIndices = &image_id;
	present_info.pResults = NULL;
	result = vkQueuePresentKHR(context->present_queue, &present_info);
	AssertVkResult(result);
	// TODO(Guigui): Recreate Swapchain if necessary

	swapchain->semaphore_id = (swapchain->semaphore_id + 1) % swapchain->image_count;

	vkDeviceWaitIdle(context->device);
}

/*


internal void CreateTLAS(VulkanContext *context, const u32 primitive_count, Primitive *primitives, Buffer *instance_data, Buffer *TLAS_buffer,
		VkAccelerationStructureKHR *TLAS) {
	Mat4 mat = {};
	mat = mat4_identity();
	mat4_transpose(&mat);
	VkTransformMatrixKHR xform = {};
	memcpy(&xform, &mat, sizeof(xform));

	VkAccelerationStructureDeviceAddressInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	info.pNext = NULL;
	info.accelerationStructure = renderer->BLAS;

	VkAccelerationStructureInstanceKHR instance = {};
	instance.transform = xform;
	instance.instanceCustomIndex = 0;
	instance.mask = 0xFF;
	instance.instanceShaderBindingTableRecordOffset = 0;
	instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	instance.accelerationStructureReference = (u64)pfn_vkGetAccelerationStructureDeviceAddressKHR(context->device, &info);

	CreateBuffer(context, sizeof(VkAccelerationStructureInstanceKHR),
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &renderer->instance_data);
	DEBUGNameBuffer(context->device, &renderer->instance_data, "Instance Data");
	UploadToBuffer(context->device, &renderer->instance_data, &instance, sizeof(VkAccelerationStructureInstanceKHR));

	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.pNext = NULL;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry.geometry.instances.pNext = NULL;
	geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	geometry.geometry.instances.data.deviceAddress = renderer->instance_data.address;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.pNext = NULL;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.srcAccelerationStructure = VK_NULL_HANDLE;
	build_info.dstAccelerationStructure = VK_NULL_HANDLE; // Set below
	build_info.geometryCount = primitive_count;
	build_info.pGeometries = &geometry;
	build_info.ppGeometries = NULL;
	build_info.scratchData.deviceAddress = NULL; // Set below

	// Get size info
	VkAccelerationStructureBuildSizesInfoKHR size_info = {};
	size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	pfn_vkGetAccelerationStructureBuildSizesKHR(
			context->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, &primitive_count, &size_info);
	CreateBuffer(context, size_info.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &renderer->TLAS_buffer);
	DEBUGNameBuffer(context->device, &renderer->TLAS_buffer, "TLAS");
	Buffer scratch;
	CreateBuffer(context, size_info.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &scratch);

	VkAccelerationStructureCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	create_info.pNext = NULL;
	create_info.createFlags = 0;
	create_info.buffer = renderer->TLAS_buffer.buffer;
	create_info.offset = 0;
	create_info.size = size_info.accelerationStructureSize;
	create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	create_info.deviceAddress = 0;
	AssertVkResult(pfn_vkCreateAccelerationStructureKHR(context->device, &create_info, NULL, &renderer->TLAS));
	DEBUGNameObject(context->device, (u64)renderer->TLAS, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, "TLAS");

	// Now build it
	build_info.dstAccelerationStructure = renderer->TLAS;
	build_info.scratchData.deviceAddress = scratch.address;

	VkAccelerationStructureBuildRangeInfoKHR range_info = { primitive_count, 0, 0, 0 };
	VkAccelerationStructureBuildRangeInfoKHR *infos[] = { &range_info };

	vkDeviceWaitIdle(context->device);

	VkCommandBuffer cmd_buffer;
	AllocateCommandBuffers(context->device, context->graphics_command_pool, 1, &cmd_buffer);
	BeginCommandBuffer(context->device, cmd_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER, NULL, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR };

	vkCmdPipelineBarrier(
			cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, NULL, 0, NULL);

	pfn_vkCmdBuildAccelerationStructuresKHR(cmd_buffer, 1, &build_info, infos);
	vkEndCommandBuffer(cmd_buffer);

	VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmd_buffer, 0, NULL };
	vkQueueSubmit(context->graphics_queue, 1, &si, VK_NULL_HANDLE);
	vkQueueWaitIdle(context->graphics_queue);

	vkFreeCommandBuffers(context->device, context->graphics_command_pool, 1, &cmd_buffer);
	DestroyBuffer(context->device, &scratch);
}

void VulkanReloadRTXShaders(VulkanContext *context, VulkanRTXRenderer *renderer)
{ vkDestroyPipeline(context->device, renderer->pipeline, NULL);
	CreateRtxPipeline(context->device, &renderer->layout,
&context->rtx_properties, &renderer->pipeline);
}
internal void WriteRTXDescriptorSets(const VkDevice device, const
VkDescriptorSet *descriptor_set, const VulkanRTXRenderer *pipeline, const
VulkanContext *context) { VkDescriptorImageInfo img_info = { VK_NULL_HANDLE,
pipeline->render_image.image_view, VK_IMAGE_LAYOUT_GENERAL
	};

	VkWriteDescriptorSetAccelerationStructureKHR as_write = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, NULL,
1, &pipeline->TLAS
	};

	VkWriteDescriptorSet app_writes[2] = {
		{ // Render image
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				NULL,
				*descriptor_set,
				0, 0, 1,
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &img_info, NULL, NULL },
		{ // Acceleration Structure
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				&as_write,
				*descriptor_set,
				1, 0, 1,
				VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
				NULL, NULL, NULL }

	};

	vkUpdateDescriptorSets(device, 2, app_writes, 0, NULL);

	u32 game_writes_count = 1;
	VkDescriptorBufferInfo bi_cam = { context->cam_buffer.buffer, 0,
VK_WHOLE_SIZE }; VkWriteDescriptorSet game_writes[1]; game_writes[0] = {
VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, NULL, descriptor_set[1], 0, 0, 1,
VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, NULL, &bi_cam, NULL };

	vkUpdateDescriptorSets(device, game_writes_count, game_writes, 0, NULL);
}

internal void CreateRTXPipelineLayout(const VkDevice device, const Image
*result_image, VulkanRTXRenderer *pipeline, GameCode *game) {
	// Set Layout

	// TODO(Guigui): TEMP
	const u32 mesh_count = 1;
	const u32 material_count = 1;

	// Our set layout
	pipeline->descriptor_set_count = 2;
	const u32 app_descriptor_count = 2;
	VkDescriptorSetLayoutBinding app_bindings[app_descriptor_count] = {};
	app_bindings[0] = { 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
VK_SHADER_STAGE_RAYGEN_BIT_KHR, NULL }; app_bindings[1] = { 1,
VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR
| VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, NULL };

	VkDescriptorSetLayoutCreateInfo app_set_create_info = {};
	app_set_create_info.sType =
VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO; app_set_create_info.pNext =
NULL; app_set_create_info.flags = 0; app_set_create_info.bindingCount =
app_descriptor_count; app_set_create_info.pBindings = app_bindings; VkResult
result = vkCreateDescriptorSetLayout(device, &app_set_create_info, NULL,
&pipeline->app_set_layout); AssertVkResult(result);

	// Get the game set layout
	const VkDescriptorSetLayoutBinding game_bindings[] = {
		{ // CAMERA MATRICES
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				1,
				VK_SHADER_STAGE_RAYGEN_BIT_KHR |
VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, NULL }, { // SCENE DESCRIPTION 1,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				1,
				VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
				NULL },
		{ // VERTEX BUFFER
				2,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				mesh_count,
				VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
				NULL },
		{ // INDEX BUFFER
				3,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				mesh_count,
				VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
				NULL },
		{ // MATERIAL BUFFER
				4,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				material_count,
				VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
				NULL }
	};
	const u32 game_descriptor_count = sizeof(game_bindings) /
sizeof(game_bindings[0]);

	VkDescriptorSetLayoutCreateInfo game_set_create_info = {};
	game_set_create_info.sType =
VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO; game_set_create_info.pNext
= NULL; game_set_create_info.flags = 0; game_set_create_info.bindingCount =
game_descriptor_count; game_set_create_info.pBindings = game_bindings; result =
vkCreateDescriptorSetLayout(device, &game_set_create_info, NULL,
&pipeline->game_set_layout); AssertVkResult(result);

	// Descriptor Pool
	// TODO(Guigui): if the game doesn't use one of these types, we get a
validation error VkDescriptorPoolSize pool_sizes[] = { {
VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0 }, { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0 },
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0 }
	};
	const u32 pool_sizes_count = sizeof(pool_sizes) / sizeof(pool_sizes[0]);

	// App
	for (u32 d = 0; d < app_descriptor_count; d++) {
		for (u32 i = 0; i < pool_sizes_count; i++) {
			if (app_bindings[d].descriptorType == pool_sizes[i].type) {
				pool_sizes[i].descriptorCount++;
			}
		}
	}

	//Game
	for (u32 d = 0; d < game_descriptor_count; d++) {
		for (u32 i = 0; i < pool_sizes_count; i++) {
			if (game_bindings[d].descriptorType == pool_sizes[i].type) {
				pool_sizes[i].descriptorCount++;
			}
		}
	}

	VkDescriptorPoolCreateInfo pool_ci = {};
	pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_ci.pNext = NULL;
	pool_ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_ci.maxSets = pipeline->descriptor_set_count;
	pool_ci.poolSizeCount = pool_sizes_count;
	pool_ci.pPoolSizes = pool_sizes;
	AssertVkResult(vkCreateDescriptorPool(device, &pool_ci, NULL,
&pipeline->descriptor_pool));

	// Descriptor Set
	VkDescriptorSetLayout set_layouts[2] = { pipeline->app_set_layout,
pipeline->game_set_layout }; VkDescriptorSetAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.descriptorPool = pipeline->descriptor_pool;
	allocate_info.descriptorSetCount = pipeline->descriptor_set_count;
	allocate_info.pSetLayouts = set_layouts;
	AssertVkResult(vkAllocateDescriptorSets(device, &allocate_info,
pipeline->descriptor_sets));

	// Push constants
	// TODO(Guigui): Currently limited to 1 PC

	VkPushConstantRange push_constant_range = { VK_SHADER_STAGE_RAYGEN_BIT_KHR |
VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 0,
sizeof(PushConstant) }; const u32 push_constant_count = 1;
	pipeline->push_constant_size = push_constant_range.size;

	VkPipelineLayoutCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.setLayoutCount = pipeline->descriptor_set_count;
	create_info.pSetLayouts = set_layouts;
	create_info.pushConstantRangeCount = push_constant_count;
	create_info.pPushConstantRanges = &push_constant_range;

	AssertVkResult(vkCreatePipelineLayout(device, &create_info, NULL,
&pipeline->layout));
}

internal void CreateBLASGLTF(VulkanContext *context, cgltf_mesh *mesh, Buffer
*buffer, Buffer *BLAS_buffer, VkAccelerationStructureKHR *BLAS) { if
(mesh->primitives_count > 1) { SDL_LogError(0, "Multiple primitives, not
supported... yet"); ASSERT(0);
	}
	cgltf_primitive *primitive = &mesh->primitives[0];

	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.pNext = NULL;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.geometry.triangles.sType =
VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	geometry.geometry.triangles.pNext = NULL;

	u32 triangle_count = 0;

	// Vertex
	for (u32 i = 0; i < primitive->attributes_count; i++) {
		if (primitive->attributes[i].type == cgltf_attribute_type_position) {
			cgltf_accessor *position_data = primitive->attributes[i].data;

			if (position_data->component_type == cgltf_component_type_r_32f) {
				geometry.geometry.triangles.vertexFormat =
VK_FORMAT_R32G32B32_SFLOAT;
			}

			geometry.geometry.triangles.vertexData.deviceAddress =
buffer->address + position_data->offset + position_data->buffer_view->offset;
			geometry.geometry.triangles.vertexStride = position_data->stride;
			geometry.geometry.triangles.maxVertex = position_data->count;
			break;
		}
	}
	// Index
	if (primitive->indices) {
		cgltf_accessor *indices = primitive->indices;
		if (indices->component_type == cgltf_component_type_r_16u) {
			geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT16;
		} else {
			ASSERT(0);
		}
		triangle_count = indices->count / 3;
		geometry.geometry.triangles.indexData.deviceAddress = buffer->address +
primitive->indices->buffer_view->offset; } else { SDL_LogError(0, "Unindexed
meshed are not supported ??, ... yet"); ASSERT(0);
	}

	geometry.geometry.triangles.transformData.deviceAddress = VK_NULL_HANDLE;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType =
VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.pNext = NULL;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	build_info.flags =
VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; build_info.mode =
VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.srcAccelerationStructure = VK_NULL_HANDLE;
	build_info.geometryCount = 1;
	build_info.pGeometries = &geometry;
	build_info.ppGeometries = NULL;
	build_info.scratchData.deviceAddress = NULL;

	// Query the build size
	VkAccelerationStructureBuildSizesInfoKHR size_info = {};
	size_info.sType =
VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	pfn_vkGetAccelerationStructureBuildSizesKHR(context->device,
VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, &triangle_count,
&size_info);

	//Alloc the scratch buffer
	Buffer scratch;
	CreateBuffer(context, size_info.buildScratchSize,
VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &scratch);
	build_info.scratchData.deviceAddress = scratch.address;

	//Alloc the blas buffer
	CreateBuffer(context, size_info.accelerationStructureSize,
VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, BLAS_buffer);
	DEBUGNameBuffer(context->device, BLAS_buffer, "BLAS");

	VkAccelerationStructureCreateInfoKHR blas_ci = {};
	blas_ci.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	blas_ci.pNext = NULL;
	blas_ci.createFlags = 0;
	blas_ci.buffer = BLAS_buffer->buffer;
	blas_ci.offset = 0;
	blas_ci.size = size_info.accelerationStructureSize; // Will be queried below
	blas_ci.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	blas_ci.deviceAddress = 0;
	VkResult result = pfn_vkCreateAccelerationStructureKHR(context->device,
&blas_ci, NULL, BLAS); AssertVkResult(result); DEBUGNameObject(context->device,
(u64)*BLAS, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, "BLAS");

	// Now build it !!
	build_info.dstAccelerationStructure = *BLAS;
	VkAccelerationStructureBuildRangeInfoKHR range_info = { triangle_count, 0,
0, 0 }; VkAccelerationStructureBuildRangeInfoKHR *infos[] = { &range_info };

	VkCommandBuffer cmd_buffer;
	AllocateCommandBuffers(context->device, context->graphics_command_pool, 1,
&cmd_buffer); BeginCommandBuffer(context->device, cmd_buffer,
VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	pfn_vkCmdBuildAccelerationStructuresKHR(cmd_buffer, 1, &build_info, infos);
	VkMemoryBarrier barrier = {
		VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		NULL,
		VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR
	};

	vkCmdPipelineBarrier(cmd_buffer,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			0,
			1, &barrier,
			0, NULL, 0, NULL);
	vkEndCommandBuffer(cmd_buffer);
	VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1,
&cmd_buffer, 0, NULL }; vkQueueSubmit(context->graphics_queue, 1, &si,
VK_NULL_HANDLE); vkQueueWaitIdle(context->graphics_queue);

	vkFreeCommandBuffers(context->device, context->graphics_command_pool, 1,
&cmd_buffer); DestroyBuffer(context->device, &scratch);
}

VulkanRTXRenderer *VulkanCreateRTXRenderer(VulkanContext *context, GameCode
*game) { VulkanRTXRenderer *renderer = (VulkanRTXRenderer
*)malloc(sizeof(VulkanRTXRenderer));

	CreateRtxRenderImage(context->device, &context->swapchain,
&context->memory_properties, &renderer->render_image);

	CreateRTXPipelineLayout(context->device, &renderer->render_image, renderer,
game); CreateRtxPipeline(context->device, &renderer->layout,
&context->rtx_properties, &renderer->pipeline);

	// SHADER BINDING TABLE
	{
		const u32 handle_size = context->rtx_properties.shaderGroupHandleSize;
		const u32 handle_size_aligned =
aligned_size(context->rtx_properties.shaderGroupHandleSize,
context->rtx_properties.shaderGroupHandleAlignment); const u32 group_count = 4;
		const u32 sbt_size = group_count * handle_size_aligned;

		CreateBuffer(context, handle_size_aligned,
VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
&renderer->rgen_sbt); DEBUGNameBuffer(context->device, &renderer->rgen_sbt,
"RGEN SBT");

		CreateBuffer(context, handle_size_aligned,
VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
&renderer->rmiss_sbt); DEBUGNameBuffer(context->device, &renderer->rmiss_sbt,
"RMISS SBT");

		CreateBuffer(context, handle_size_aligned,
VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
&renderer->rchit_sbt); DEBUGNameBuffer(context->device, &renderer->rchit_sbt,
"RCHIT SBT");

		u8 *data = (u8 *)malloc(sbt_size);

		VkResult result =
pfn_vkGetRayTracingShaderGroupHandlesKHR(context->device, renderer->pipeline, 0,
group_count, sbt_size, data); AssertVkResult(result);

		UploadToBuffer(context->device, &renderer->rgen_sbt, data,
handle_size_aligned); UploadToBuffer(context->device, &renderer->rmiss_sbt, data
+ handle_size_aligned, handle_size_aligned * 2); UploadToBuffer(context->device,
&renderer->rchit_sbt, data + (3 * handle_size_aligned), handle_size_aligned);

		renderer->strides[0] = { renderer->rgen_sbt.address,
handle_size_aligned, handle_size_aligned }; renderer->strides[1] = {
renderer->rmiss_sbt.address, handle_size_aligned, handle_size_aligned };
		renderer->strides[2] = { renderer->rchit_sbt.address,
handle_size_aligned, handle_size_aligned }; renderer->strides[3] = { 0u, 0u, 0u
}; free(data);
	}
#if 1
	u32 vtx_count = 0;
	u32 idx_count = 0;
	game->GetScene(&vtx_count, NULL, &idx_count, NULL);
	vec3 *vertices = (vec3 *)calloc(vtx_count, sizeof(vec3));
	u32 *indices = (u32 *)calloc(idx_count, sizeof(u32));
	game->GetScene(&vtx_count, vertices, &idx_count, indices);

	CreateBuffer(context, sizeof(vec3) * vtx_count,
VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
&context->vertex_buffer); DEBUGNameBuffer(context->device,
&context->vertex_buffer, "Vertices"); UploadToBuffer(context->device,
&context->vertex_buffer, (void*)vertices, sizeof(vec3) * vtx_count);

	CreateBuffer(context, sizeof(u32) * idx_count,
VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
&context->index_buffer); DEBUGNameBuffer(context->device,
&context->index_buffer, "Indices"); UploadToBuffer(context->device,
&context->index_buffer, (void*)indices, sizeof(u32) * idx_count);

	free(vertices);
	free(indices);

	const u32 primitive_count = idx_count / 3;
	CreateBLAS(context, &context->vertex_buffer, vtx_count,
&context->index_buffer, primitive_count); #endif
	// NEW METHOD; from gltf
#if 0
	cgltf_options options = { 0 };
	cgltf_data *data = NULL;
	const char *file = "resources/models/box/Box.gltf";
	cgltf_result result = cgltf_parse_file(&options, file, &data);
	if (result != cgltf_result_success) {
		SDL_LogError(0, "Error reading scene");
		ASSERT(0);
	}
	cgltf_load_buffers(&options, data, file);

	CreateBuffer(context, data->buffers[0].size,
VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
&renderer->gltf_buffer); DEBUGNameBuffer(context->device,
&renderer->gltf_buffer, "GLTF"); UploadToBuffer(context->device,
&renderer->gltf_buffer, data->buffers[0].data, data->buffers[0].size);

	if (data->meshes_count > 1) {
		SDL_LogError(0, "Multiple meshes not supported, yet");
		ASSERT(0);
	}
	CreateBLASGLTF(context, &data->meshes[0], &renderer->gltf_buffer,
&renderer->BLAS_buffer, &renderer->BLAS);

	cgltf_free(data);
#endif
	CreateTLAS(context, renderer);

	WriteRTXDescriptorSets(context->device, renderer->descriptor_sets, renderer,
context);

	return renderer;
}

void VulkanDestroyRTXRenderer(VulkanContext *context, VulkanRTXRenderer
*renderer) { DestroyBuffer(context->device, &renderer->gltf_buffer);

	DestroyBuffer(context->device, &renderer->TLAS_buffer);
	pfn_vkDestroyAccelerationStructureKHR(context->device, renderer->TLAS,
NULL); DestroyBuffer(context->device, &renderer->instance_data); #if 0
	DestroyBuffer(context->device, &renderer->vertex_buffer);
	DestroyBuffer(context->device, &renderer->index_buffer);
#endif
	DestroyBuffer(context->device, &renderer->BLAS_buffer);
	pfn_vkDestroyAccelerationStructureKHR(context->device, renderer->BLAS,
NULL);

	DestroyBuffer(context->device, &renderer->rgen_sbt);
	DestroyBuffer(context->device, &renderer->rmiss_sbt);
	DestroyBuffer(context->device, &renderer->rchit_sbt);
	DestroyImage(context->device, &renderer->render_image);

	vkFreeDescriptorSets(context->device, renderer->descriptor_pool, 2,
renderer->descriptor_sets); vkDestroyDescriptorPool(context->device,
renderer->descriptor_pool, NULL);

	vkDestroyDescriptorSetLayout(context->device, renderer->app_set_layout,
NULL); vkDestroyDescriptorSetLayout(context->device, renderer->game_set_layout,
NULL); vkDestroyPipelineLayout(context->device, renderer->layout, NULL);
	vkDestroyPipeline(context->device, renderer->pipeline, NULL);

	free(renderer);
}
*/