
/*
 === TODO ===
 CRITICAL

 MAJOR
 - Séparer le remplissage des infos de layout du build. Pour pouvoir faire genre
 : je mets les infos de la scène, je mets les infos du renderer (rtx ou raster),
 je build le layout BACKLOG

 IMPROVEMENTS

*/

#include <sl3dge/types.h>
#include <sl3dge/debug.h>
#include "vulkan_layer.h"

internal void CreateSceneDescriptorSet(VulkanContext *context, Scene *scene, VkDescriptorSetLayout *set_layout, VkDescriptorSet *descriptor_set) {
	const VkDescriptorSetLayoutBinding bindings[] = {
		{ // CAMERA MATRICES
				0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
				NULL },
		{ // MATERIALS
				1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
		{ // TEXTURES
				2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, scene->textures_count, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
		{ // SHADOWMAP READ
				3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL }
	};
	const u32 descriptor_count = sizeof(bindings) / sizeof(bindings[0]);

	VkDescriptorSetLayoutCreateInfo game_set_create_info = {};
	game_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	game_set_create_info.pNext = NULL;
	game_set_create_info.flags = 0;
	game_set_create_info.bindingCount = descriptor_count;
	game_set_create_info.pBindings = bindings;
	AssertVkResult(vkCreateDescriptorSetLayout(context->device, &game_set_create_info, NULL, set_layout));

	// Descriptor Set
	VkDescriptorSetAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.pNext = NULL;
	allocate_info.descriptorPool = context->descriptor_pool;
	allocate_info.descriptorSetCount = 1;
	allocate_info.pSetLayouts = set_layout;
	AssertVkResult(vkAllocateDescriptorSets(context->device, &allocate_info, descriptor_set));

	// Writes
	const u32 static_writes_count = 3;
	VkWriteDescriptorSet static_writes[static_writes_count];

	VkDescriptorBufferInfo bi_cam = { context->cam_buffer.buffer, 0, VK_WHOLE_SIZE };
	static_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	static_writes[0].pNext = NULL;
	static_writes[0].dstSet = *descriptor_set;
	static_writes[0].dstBinding = 0;
	static_writes[0].dstArrayElement = 0;
	static_writes[0].descriptorCount = 1;
	static_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	static_writes[0].pImageInfo = NULL;
	static_writes[0].pBufferInfo = &bi_cam;
	static_writes[0].pTexelBufferView = NULL;

	VkDescriptorBufferInfo materials = { scene->mat_buffer.buffer, 0, VK_WHOLE_SIZE };
	static_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	static_writes[1].pNext = NULL;
	static_writes[1].dstSet = *descriptor_set;
	static_writes[1].dstBinding = 1;
	static_writes[1].dstArrayElement = 0;
	static_writes[1].descriptorCount = 1;
	static_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	static_writes[1].pImageInfo = NULL;
	static_writes[1].pBufferInfo = &materials;
	static_writes[1].pTexelBufferView = NULL;

	VkDescriptorImageInfo shadowmap_info = { context->shadowmap_sampler, context->shadowmap.image_view,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
	static_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	static_writes[2].pNext = NULL;
	static_writes[2].dstSet = *descriptor_set;
	static_writes[2].dstBinding = 3;
	static_writes[2].dstArrayElement = 0;
	static_writes[2].descriptorCount = 1;
	static_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	static_writes[2].pImageInfo = &shadowmap_info;
	static_writes[2].pBufferInfo = NULL;
	static_writes[2].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(context->device, static_writes_count, static_writes, 0, NULL);

	if (scene->textures_count != 0) {
		const u32 nb_tex = scene->textures_count;
		u32 nb_info = nb_tex > 0 ? nb_tex : 1;

		VkDescriptorImageInfo *images_info = (VkDescriptorImageInfo *)calloc(nb_info, sizeof(VkDescriptorImageInfo));

		for (u32 i = 0; i < nb_info; ++i) {
			images_info[i].sampler = context->texture_sampler;
			images_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (nb_tex == 0) {
				images_info[i].imageView = VK_NULL_HANDLE;
				break;
			}
			images_info[i].imageView = scene->textures[i].image_view;
		}

		VkWriteDescriptorSet textures_buffer = {};
		textures_buffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		textures_buffer.pNext = NULL;
		textures_buffer.dstSet = *descriptor_set;
		textures_buffer.dstBinding = 2;
		textures_buffer.dstArrayElement = 0;
		textures_buffer.descriptorCount = nb_info;
		textures_buffer.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textures_buffer.pImageInfo = images_info;
		textures_buffer.pBufferInfo = NULL;
		textures_buffer.pTexelBufferView = NULL;

		vkUpdateDescriptorSets(context->device, 1, &textures_buffer, 0, NULL);
		free(images_info);
	}
}

internal void BuildLayout(VkDevice device, const u32 set_layout_count, VkDescriptorSetLayout *set_layouts, VkPipelineLayout *layout) {
	// Push constants
	VkPushConstantRange push_constant_range = { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant) };
	const u32 push_constant_count = 1;

	// Layout
	VkPipelineLayoutCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.setLayoutCount = set_layout_count;
	create_info.pSetLayouts = set_layouts;
	create_info.pushConstantRangeCount = push_constant_count;
	create_info.pPushConstantRanges = &push_constant_range;

	AssertVkResult(vkCreatePipelineLayout(device, &create_info, NULL, layout));
}

internal void CreateScenePipeline(const VkDevice device, const VkPipelineLayout layout, Swapchain *swapchain, VkRenderPass render_pass,
		VkSampleCountFlagBits sample_count, VkPipeline *pipeline) {
	VkGraphicsPipelineCreateInfo pipeline_ci = {};
	pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_ci.pNext = NULL;
	pipeline_ci.flags = 0;

	VkPipelineShaderStageCreateInfo stages_ci[2];
	stages_ci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages_ci[0].pNext = NULL;
	stages_ci[0].flags = 0;
	stages_ci[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	CreateVkShaderModule("resources/shaders/general.vert.spv", device, &stages_ci[0].module);
	stages_ci[0].pName = "main";
	stages_ci[0].pSpecializationInfo = NULL;

	stages_ci[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages_ci[1].pNext = NULL;
	stages_ci[1].flags = 0;
	stages_ci[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	CreateVkShaderModule("resources/shaders/general.frag.spv", device, &stages_ci[1].module);
	stages_ci[1].pName = "main";
	stages_ci[1].pSpecializationInfo = NULL;

	pipeline_ci.stageCount = 2;
	pipeline_ci.pStages = stages_ci;

	VkPipelineVertexInputStateCreateInfo vertex_input = {};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input.pNext = NULL;
	vertex_input.flags = 0;

	vertex_input.vertexBindingDescriptionCount = 1;
	VkVertexInputBindingDescription vtx_input_binding = {};
	vtx_input_binding.binding = 0;
	vtx_input_binding.stride = sizeof(Vertex);
	vtx_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertex_input.pVertexBindingDescriptions = &vtx_input_binding;

	vertex_input.vertexAttributeDescriptionCount = 3;
	VkVertexInputAttributeDescription vtx_descriptions[] = {
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) },
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) },
		{ 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) },
	};
	vertex_input.pVertexAttributeDescriptions = vtx_descriptions;

	pipeline_ci.pVertexInputState = &vertex_input;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
	input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state.pNext = NULL;
	input_assembly_state.flags = 0;
	input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state.primitiveRestartEnable = VK_FALSE;
	pipeline_ci.pInputAssemblyState = &input_assembly_state;

	pipeline_ci.pTessellationState = NULL;

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = NULL;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	VkViewport viewport = {};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = swapchain->extent.width;
	viewport.height = swapchain->extent.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchain->extent;
	viewport_state.pScissors = &scissor;
	pipeline_ci.pViewportState = &viewport_state;

	VkPipelineRasterizationStateCreateInfo rasterization_state = {};
	rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state.pNext = NULL;
	rasterization_state.flags = 0;
	rasterization_state.depthClampEnable = VK_FALSE;
	rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state.cullMode = VK_CULL_MODE_NONE;
	rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization_state.depthBiasEnable = VK_FALSE;
	rasterization_state.depthBiasConstantFactor = 0.f;
	rasterization_state.depthBiasClamp = 0.f;
	rasterization_state.depthBiasSlopeFactor = 0.f;
	rasterization_state.lineWidth = 1.f;
	pipeline_ci.pRasterizationState = &rasterization_state;

	VkPipelineMultisampleStateCreateInfo multisample_state = {};
	multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state.pNext = NULL;
	multisample_state.flags = 0;
	multisample_state.rasterizationSamples = sample_count;
	multisample_state.sampleShadingEnable = VK_FALSE;
	multisample_state.minSampleShading = 0.f;
	multisample_state.pSampleMask = 0;
	multisample_state.alphaToCoverageEnable = VK_FALSE;
	multisample_state.alphaToOneEnable = VK_FALSE;
	pipeline_ci.pMultisampleState = &multisample_state;

	VkPipelineDepthStencilStateCreateInfo stencil_state = {};
	stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	stencil_state.pNext = NULL;
	stencil_state.flags = 0;
	stencil_state.depthTestEnable = VK_TRUE;
	stencil_state.depthWriteEnable = VK_TRUE;
	stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;
	stencil_state.depthBoundsTestEnable = VK_FALSE;
	stencil_state.stencilTestEnable = VK_FALSE;
	stencil_state.front = {};
	stencil_state.back = {};
	stencil_state.minDepthBounds = 0.0f;
	stencil_state.maxDepthBounds = 1.0f;
	pipeline_ci.pDepthStencilState = &stencil_state;

	VkPipelineColorBlendStateCreateInfo color_blend_state = {};
	color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state.pNext = NULL;
	color_blend_state.flags = 0;
	color_blend_state.logicOpEnable = VK_FALSE;
	color_blend_state.logicOp = VK_LOGIC_OP_COPY;

	VkPipelineColorBlendAttachmentState color_blend_attachement = {};
	color_blend_attachement.blendEnable = VK_FALSE;
	color_blend_attachement.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	/*
		VkBlendFactor            srcColorBlendFactor;
		VkBlendFactor            dstColorBlendFactor;
		VkBlendOp                colorBlendOp;
		VkBlendFactor            srcAlphaBlendFactor;
		VkBlendFactor            dstAlphaBlendFactor;
		VkBlendOp                alphaBlendOp;

	*/
	color_blend_state.attachmentCount = 1;
	color_blend_state.pAttachments = &color_blend_attachement;
	color_blend_state.blendConstants[0] = 0.0f;
	color_blend_state.blendConstants[1] = 0.0f;
	color_blend_state.blendConstants[2] = 0.0f;
	color_blend_state.blendConstants[3] = 0.0f;
	pipeline_ci.pColorBlendState = &color_blend_state;

	pipeline_ci.pDynamicState = NULL; // TODO(Guigui): look at this
	pipeline_ci.layout = layout;

	pipeline_ci.renderPass = render_pass;
	pipeline_ci.subpass = 0;
	pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_ci.basePipelineIndex = 0;

	// TODO: handle pipeline caching
	AssertVkResult(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, pipeline));

	vkDeviceWaitIdle(device);
	vkDestroyShaderModule(device, pipeline_ci.pStages[0].module, NULL);
	vkDestroyShaderModule(device, pipeline_ci.pStages[1].module, NULL);
}

internal void VulkanUpdateDescriptors(VulkanContext *context, GameData *game_data) {
	UploadToBuffer(context->device, &context->cam_buffer, &game_data->matrices, sizeof(game_data->matrices));
}

DLL_EXPORT Scene *VulkanLoadScene(const char *file, VulkanContext *context) {
	double start = SDL_GetPerformanceCounter();

	Scene *scene = (Scene *)malloc(sizeof(Scene));
	*scene = {};

	char *directory;
	const char *last_sep = strrchr(file, '/');
	u32 size = last_sep - file;
	directory = (char *)calloc(size + 2, sizeof(char));
	strncpy(directory, file, size);
	directory[size] = '/';
	directory[size + 1] = '\0';

	cgltf_data *data;
	cgltf_options options = { 0 };
	cgltf_result result = cgltf_parse_file(&options, file, &data);
	if (result != cgltf_result_success) {
		SDL_LogError(0, "Error reading scene");
		ASSERT(0);
	}

	cgltf_load_buffers(&options, data, file);

	// Transforms
	scene->nodes_count = data->nodes_count;
	scene->transforms = (Mat4 *)calloc(scene->nodes_count, sizeof(Mat4));
	GLTFLoadTransforms(data, scene->transforms);

	// Primitives
	scene->total_primitives_count = 0;
	for (u32 m = 0; m < data->meshes_count; ++m) {
		scene->total_primitives_count += data->meshes[m].primitives_count;
	};
	scene->primitives = (Primitive *)calloc(scene->total_primitives_count, sizeof(Primitive));

	u32 i = 0;
	for (u32 m = 0; m < data->meshes_count; ++m) {
		for (u32 p = 0; p < data->meshes[m].primitives_count; p++) {
			cgltf_primitive *prim = &data->meshes[m].primitives[p];

			Primitive *primitive = &scene->primitives[i];

			primitive->vertex_count = (u32)prim->attributes[0].data->count;
			primitive->vertex_offset = scene->total_vertex_count;
			scene->total_vertex_count += (u32)prim->attributes[0].data->count;

			primitive->index_count += prim->indices->count;
			primitive->index_offset = scene->total_index_count;
			scene->total_index_count += (u32)prim->indices->count;

			primitive->material_id = GLTFGetMaterialID(prim->material);

			++i;
		}
	}

	// Vertex & Index Buffer
	CreateBuffer(context->device, &context->memory_properties, scene->total_vertex_count * sizeof(Vertex),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &scene->vtx_buffer);
	CreateBuffer(context->device, &context->memory_properties, scene->total_index_count * sizeof(u32),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &scene->idx_buffer);
	DEBUGNameBuffer(context->device, &scene->vtx_buffer, "GLTF VTX");
	DEBUGNameBuffer(context->device, &scene->idx_buffer, "GLTF IDX");
	void *mapped_vtx_buffer;
	void *mapped_idx_buffer;
	MapBuffer(context->device, &scene->vtx_buffer, &mapped_vtx_buffer);
	MapBuffer(context->device, &scene->idx_buffer, &mapped_idx_buffer);

	i = 0;
	for (u32 m = 0; m < data->meshes_count; ++m) {
		for (u32 p = 0; p < data->meshes[m].primitives_count; ++p) {
			GLTFLoadVertexAndIndexBuffer(&data->meshes[m].primitives[p], &scene->primitives[i], mapped_vtx_buffer, mapped_idx_buffer);
			++i;
		}
	}
	UnmapBuffer(context->device, &scene->vtx_buffer);
	UnmapBuffer(context->device, &scene->idx_buffer);

	// Materials
	scene->materials_count = data->materials_count;
	CreateBuffer(context->device, &context->memory_properties, scene->materials_count * sizeof(Material), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &scene->mat_buffer);
	DEBUGNameBuffer(context->device, &scene->mat_buffer, "GLTF MATS");
	void *mapped_mat_buffer;
	MapBuffer(context->device, &scene->mat_buffer, &mapped_mat_buffer);
	GLTFLoadMaterialBuffer(data, (Material *)mapped_mat_buffer);
	UnmapBuffer(context->device, &scene->mat_buffer);

	// Textures
	SDL_Log("Loading textures...");
	scene->textures_count = data->textures_count;
	if (data->textures_count > 0) {
		scene->textures = (Image *)calloc(data->textures_count, sizeof(Image));

		SDL_Surface **surfaces = (SDL_Surface **)calloc(data->textures_count, sizeof(SDL_Surface *));
		Buffer *image_buffers = (Buffer *)calloc(data->textures_count, sizeof(Buffer));
		VkCommandBuffer *cmds = (VkCommandBuffer *)calloc(data->textures_count, sizeof(VkCommandBuffer));

		AllocateCommandBuffers(context->device, context->graphics_command_pool, data->textures_count, cmds);

		for (u32 i = 0; i < data->textures_count; ++i) {
			char *image_path = data->textures[i].image->uri;
			ASSERT_MSG(image_path, "Attempting to load an embedded texture. "
								   "This isn't supported yet");
			u32 file_path_length = strlen(directory) + strlen(image_path) + 1;
			char *full_image_path = (char *)calloc(file_path_length, sizeof(char *));
			strcat(full_image_path, directory);
			strcat(full_image_path, image_path);

			SDL_Surface *temp_surf = IMG_Load(full_image_path);
			if (!temp_surf) {
				SDL_LogError(0, IMG_GetError());
			}

			if (temp_surf->format->format != SDL_PIXELFORMAT_ABGR8888) {
				surfaces[i] = SDL_ConvertSurfaceFormat(temp_surf, SDL_PIXELFORMAT_ABGR8888, 0);
				if (!surfaces[i]) {
					SDL_LogError(0, SDL_GetError());
				}
				SDL_FreeSurface(temp_surf);
			} else {
				surfaces[i] = temp_surf;
			}
			free(full_image_path);
		}

		for (u32 i = 0; i < data->textures_count; ++i) {
			VkDeviceSize image_size = surfaces[i]->h * surfaces[i]->pitch;
			VkExtent2D extent = { (u32)surfaces[i]->h, (u32)surfaces[i]->w };

			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
			if (data->textures[i].type == cgltf_texture_type_base_color)
				format = VK_FORMAT_R8G8B8A8_SRGB;

			CreateImage(context->device, &context->memory_properties, format, extent, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &scene->textures[i]);
			DEBUGNameImage(context->device, &scene->textures[i], data->textures[i].image->uri);
			CreateBuffer(context->device, &context->memory_properties, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &image_buffers[i]);
			SDL_LockSurface(surfaces[i]);
			UploadToBuffer(context->device, &image_buffers[i], surfaces[i]->pixels, image_size);
			SDL_UnlockSurface(surfaces[i]);
			BeginCommandBuffer(context->device, cmds[i], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			CopyBufferToImage(cmds[i], extent, surfaces[i]->pitch, &image_buffers[i], &scene->textures[i]);
			vkEndCommandBuffer(cmds[i]);
			VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmds[i], 0, NULL };
			vkQueueSubmit(context->graphics_queue, 1, &si, VK_NULL_HANDLE);
		}

		vkQueueWaitIdle(context->graphics_queue);

		vkFreeCommandBuffers(context->device, context->graphics_command_pool, data->textures_count, cmds);
		free(cmds);
		for (u32 i = 0; i < data->textures_count; ++i) {
			SDL_FreeSurface(surfaces[i]);
			DestroyBuffer(context->device, &image_buffers[i]);
		}
		free(surfaces);
		free(image_buffers);
	}

	SDL_Log("Creating Descriptors...");
	scene->descriptor_set_count = 1;
	scene->set_layouts = (VkDescriptorSetLayout *)calloc(scene->descriptor_set_count, sizeof(VkDescriptorSetLayout));
	scene->descriptor_sets = (VkDescriptorSet *)calloc(scene->descriptor_set_count, sizeof(VkDescriptorSet));

	CreateSceneDescriptorSet(context, scene, &scene->set_layouts[0], &scene->descriptor_sets[0]);

	BuildLayout(context->device, scene->descriptor_set_count, scene->set_layouts, &scene->layout);
	CreateScenePipeline(context->device, scene->layout, &context->swapchain, context->render_pass, context->msaa_level, &scene->pipeline);

	cgltf_free(data);
	free(directory);

	double end = SDL_GetPerformanceCounter();
	SDL_Log("Scene loaded in : %.2fms", (double)((end - start) * 1000) / SDL_GetPerformanceFrequency());

	return scene;
}

DLL_EXPORT void VulkanFreeScene(VulkanContext *context, Scene *scene) {
	vkFreeDescriptorSets(context->device, context->descriptor_pool, scene->descriptor_set_count, scene->descriptor_sets);
	free(scene->descriptor_sets);

	for (u32 i = 0; i < scene->descriptor_set_count; ++i) {
		vkDestroyDescriptorSetLayout(context->device, scene->set_layouts[i], 0);
	}
	free(scene->set_layouts);
	vkDestroyPipelineLayout(context->device, scene->layout, 0);

	vkDestroyPipeline(context->device, scene->pipeline, NULL);

	for (u32 i = 0; i < scene->textures_count; ++i) {
		DestroyImage(context->device, &scene->textures[i]);
	}
	free(scene->textures);

	free(scene->primitives);

	DestroyBuffer(context->device, &scene->vtx_buffer);
	DestroyBuffer(context->device, &scene->idx_buffer);
	DestroyBuffer(context->device, &scene->mat_buffer);
	free(scene->transforms);

	free(scene);
}