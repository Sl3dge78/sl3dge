#include <vulkan/vulkan.h>

#include <sl3dge/sl3dge.h>
#include "vulkan_types.h"

inline VkPipelineVertexInputStateCreateInfo
PipelineGetDefaultVertexInputState(const VkVertexInputBindingDescription *vtx_input_binding,
                                   const u32 vtx_desc_count,
                                   const VkVertexInputAttributeDescription *vtx_descriptions) {
    VkPipelineVertexInputStateCreateInfo vertex_input = {};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.pNext = NULL;
    vertex_input.flags = 0;

    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = vtx_input_binding;

    vertex_input.vertexAttributeDescriptionCount = 3;

    vertex_input.pVertexAttributeDescriptions = vtx_descriptions;
    return vertex_input;
}

inline VkPipelineInputAssemblyStateCreateInfo PipelineGetDefaultInputAssemblyState() {
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.pNext = NULL;
    input_assembly_state.flags = 0;
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;
    return input_assembly_state;
}

inline VkPipelineViewportStateCreateInfo PipelineGetDefaultViewportState(const u32 viewport_count,
                                                                         const VkViewport *viewport,
                                                                         const u32 scissor_count,
                                                                         const VkRect2D *scissor) {
    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = NULL;
    viewport_state.flags = 0;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = scissor;
    return viewport_state;
}

inline VkPipelineRasterizationStateCreateInfo PipelineGetDefaultRasterizationState() {
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
    return rasterization_state;
}

inline VkPipelineMultisampleStateCreateInfo
PipelineGetDefaultMultisampleState(const VkSampleCountFlagBits sample_count) {
    VkPipelineMultisampleStateCreateInfo multisample_state;
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.pNext = NULL;
    multisample_state.flags = 0;
    multisample_state.rasterizationSamples = sample_count;
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 0.f;
    multisample_state.pSampleMask = 0;
    multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.alphaToOneEnable = VK_FALSE;
    return multisample_state;
}

inline VkPipelineDepthStencilStateCreateInfo PipelineGetDefaultDepthStencilState() {
    VkPipelineDepthStencilStateCreateInfo stencil_state;
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
    return stencil_state;
}

inline VkPipelineColorBlendStateCreateInfo
PipelineGetDefaultColorBlendState(const u32 attachement_cout,
                                  const VkPipelineColorBlendAttachmentState *attachements) {
    VkPipelineColorBlendStateCreateInfo color_blend_state;
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.pNext = NULL;
    color_blend_state.flags = 0;
    color_blend_state.logicOpEnable = VK_FALSE;
    color_blend_state.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state.attachmentCount = attachement_cout;
    color_blend_state.pAttachments = attachements;
    color_blend_state.blendConstants[0] = 0.0f;
    color_blend_state.blendConstants[1] = 0.0f;
    color_blend_state.blendConstants[2] = 0.0f;
    color_blend_state.blendConstants[3] = 0.0f;
    return color_blend_state;
}

void PipelineCreateDefault(VkDevice device,
                           const char *vertex_shader,
                           const char *fragment_shader,
                           const VkExtent2D *extent,
                           const VkSampleCountFlagBits sample_count,
                           const VkPipelineLayout layout,
                           const VkRenderPass render_pass,
                           VkPipeline *pipeline) {
    VkGraphicsPipelineCreateInfo pipeline_ci;
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.pNext = NULL;
    pipeline_ci.flags = 0;

    VkPipelineShaderStageCreateInfo stages_ci[2];
    stages_ci[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages_ci[0].pNext = NULL;
    stages_ci[0].flags = 0;
    stages_ci[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    CreateVkShaderModule(vertex_shader, device, &stages_ci[0].module);
    stages_ci[0].pName = "main";
    stages_ci[0].pSpecializationInfo = NULL;

    stages_ci[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages_ci[1].pNext = NULL;
    stages_ci[1].flags = 0;
    stages_ci[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    CreateVkShaderModule(fragment_shader, device, &stages_ci[1].module);
    stages_ci[1].pName = "main";
    stages_ci[1].pSpecializationInfo = NULL;
    pipeline_ci.stageCount = 2;

    pipeline_ci.pStages = stages_ci;

    VkVertexInputBindingDescription vtx_input_binding = {};
    vtx_input_binding.binding = 0;
    vtx_input_binding.stride = sizeof(Vertex);
    vtx_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription vtx_descriptions[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
    };
    VkPipelineVertexInputStateCreateInfo vertex_input =
        PipelineGetDefaultVertexInputState(&vtx_input_binding, 3, vtx_descriptions);
    pipeline_ci.pVertexInputState = &vertex_input;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state =
        PipelineGetDefaultInputAssemblyState();
    pipeline_ci.pInputAssemblyState = &input_assembly_state;

    VkViewport viewport;
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = extent->width;
    viewport.height = extent->height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = *extent;
    VkPipelineViewportStateCreateInfo viewport_state =
        PipelineGetDefaultViewportState(1, &viewport, 1, &scissor);
    pipeline_ci.pViewportState = &viewport_state;

    VkPipelineRasterizationStateCreateInfo rasterization_state =
        PipelineGetDefaultRasterizationState();
    pipeline_ci.pRasterizationState = &rasterization_state;

    VkPipelineMultisampleStateCreateInfo multisample_state =
        PipelineGetDefaultMultisampleState(sample_count);
    pipeline_ci.pMultisampleState = &multisample_state;

    VkPipelineDepthStencilStateCreateInfo stencil_state = PipelineGetDefaultDepthStencilState();
    pipeline_ci.pDepthStencilState = &stencil_state;

    VkPipelineColorBlendAttachmentState color_blend_attachement = {};
    color_blend_attachement.blendEnable = VK_FALSE;
    color_blend_attachement.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo color_blend_state =
        PipelineGetDefaultColorBlendState(1, &color_blend_attachement);
    pipeline_ci.pColorBlendState = &color_blend_state;

    pipeline_ci.pDynamicState = NULL; // TODO(Guigui): look at this
    pipeline_ci.layout = layout;

    pipeline_ci.renderPass = render_pass;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_ci.basePipelineIndex = 0;

    // TODO: handle pipeline caching
    AssertVkResult(
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, pipeline));

    vkDeviceWaitIdle(device);
    vkDestroyShaderModule(device, pipeline_ci.pStages[0].module, NULL);
    vkDestroyShaderModule(device, pipeline_ci.pStages[1].module, NULL);
}
