
/*
 === TODO
 ===
 CRITICAL

 MAJOR
 - Séparer le remplissage des infos  de layout  du build. Pour pouvoir faire genre : je mets les
 infos de la scène, je mets les infos du renderer (rtx ou raster), je build le layout
 BACKLOG

 IMPROVEMENTS

*/

#include <sl3dge/sl3dge.h>
#include "vulkan_types.h"
#include "vulkan_pipeline.cpp"

internal void CreateSceneDescriptorSet(VulkanContext *context,
                                       Scene *scene,
                                       VkDescriptorSetLayout *set_layout,
                                       VkDescriptorSet *descriptor_set) {
    const VkDescriptorSetLayoutBinding bindings[] = {
        {// CAMERA MATRICES
         0,
         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         1,
         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
         NULL},
        {// MATERIALS
         1,
         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         1,
         VK_SHADER_STAGE_FRAGMENT_BIT,
         NULL},
        {// TEXTURES
         2,
         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         scene->textures_count,
         VK_SHADER_STAGE_FRAGMENT_BIT,
         NULL},
        {// SHADOWMAP READ
         3,
         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         1,
         VK_SHADER_STAGE_FRAGMENT_BIT,
         NULL}};
    const u32 descriptor_count = sizeof(bindings) / sizeof(bindings[0]);

    VkDescriptorSetLayoutCreateInfo game_set_create_info = {};
    game_set_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    game_set_create_info.pNext = NULL;
    game_set_create_info.flags = 0;
    game_set_create_info.bindingCount = descriptor_count;
    game_set_create_info.pBindings = bindings;
    AssertVkResult(
        vkCreateDescriptorSetLayout(context->device, &game_set_create_info, NULL, set_layout));

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

    VkDescriptorBufferInfo bi_cam = {context->cam_buffer.buffer, 0, VK_WHOLE_SIZE};
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

    VkDescriptorBufferInfo materials = {scene->mat_buffer.buffer, 0, VK_WHOLE_SIZE};
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

    VkDescriptorImageInfo image_info = {context->shadowmap_sampler,
                                        context->shadowmap.image_view,
                                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
    static_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    static_writes[2].pNext = NULL;
    static_writes[2].dstSet = *descriptor_set;
    static_writes[2].dstBinding = 3;
    static_writes[2].dstArrayElement = 0;
    static_writes[2].descriptorCount = 1;
    static_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    static_writes[2].pImageInfo = &image_info;
    static_writes[2].pBufferInfo = NULL;
    static_writes[2].pTexelBufferView = NULL;

    vkUpdateDescriptorSets(context->device, static_writes_count, static_writes, 0, NULL);

    if(scene->textures_count != 0) {
        const u32 nb_tex = scene->textures_count;
        u32 nb_info = nb_tex > 0 ? nb_tex : 1;

        VkDescriptorImageInfo *images_info =
            (VkDescriptorImageInfo *)calloc(nb_info, sizeof(VkDescriptorImageInfo));

        for(u32 i = 0; i < nb_info; ++i) {
            images_info[i].sampler = context->texture_sampler;
            images_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            if(nb_tex == 0) {
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

internal void BuildLayout(VkDevice device,
                          const u32 set_layout_count,
                          VkDescriptorSetLayout *set_layouts,
                          VkPipelineLayout *layout) {
}

internal void VulkanUpdateDescriptors(VulkanContext *context, GameData *game_data) {
    UploadToBuffer(
        context->device, &context->cam_buffer, &game_data->matrices, sizeof(game_data->matrices));
}

internal void BuildScene(VulkanContext *context, Scene *scene) {
    SDL_Log("Creating Descriptors...");
    scene->descriptor_set_count = 1;
    scene->set_layouts =
        (VkDescriptorSetLayout *)calloc(scene->descriptor_set_count, sizeof(VkDescriptorSetLayout));
    scene->descriptor_sets =
        (VkDescriptorSet *)calloc(scene->descriptor_set_count, sizeof(VkDescriptorSet));

    CreateSceneDescriptorSet(context, scene, &scene->set_layouts[0], &scene->descriptor_sets[0]);

    { // Build layout
        // Push constants
        VkPushConstantRange push_constant_range = {
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant)};
        const u32 push_constant_count = 1;

        // Layout
        VkPipelineLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.pNext = NULL;
        create_info.flags = 0;
        create_info.setLayoutCount = scene->descriptor_set_count;
        create_info.pSetLayouts = scene->set_layouts;
        create_info.pushConstantRangeCount = push_constant_count;
        create_info.pPushConstantRanges = &push_constant_range;

        AssertVkResult(vkCreatePipelineLayout(context->device, &create_info, NULL, &scene->layout));
    }
    Pipeline::CreateDefault(context->device,
                            "resources/shaders/general.vert.spv",
                            "resources/shaders/general.frag.spv",
                            &context->swapchain.extent,
                            context->msaa_level,
                            scene->layout,
                            context->render_pass,
                            &scene->pipeline);
}

DLL_EXPORT Scene *VulkanCreateScene(VulkanContext *context) {
    Scene *scene = (Scene *)malloc(sizeof(Scene));

    const u32 MAX_MATERIALS = 128;
    const u32 MAX_TEXTURES = 128;
    CreateBuffer(context->device,
                 &context->memory_properties,
                 128 * sizeof(Material),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &scene->mat_buffer);
    DEBUGNameBuffer(context->device, &scene->mat_buffer, "SCENE MATS");

    scene->textures = (Image *)calloc(MAX_TEXTURES, sizeof(Image));
    scene->textures_count = 0;

    // TODO TEMPORARY
    scene->meshes = (Mesh **)calloc(1, sizeof(Mesh *));
    scene->mesh_count = 0;
    SDL_Log("Loading mesh...");
    const char *scene_path = "resources/3d/Motorcycle/motorcycle.gltf";
    scene->meshes[0] = LoadMesh(scene_path, context, scene);
    scene->mesh_count++;
    scene->meshes[0]->instance_count = 1;
    scene->meshes[0]->instance_transforms = (Mat4 *)calloc(1, sizeof(Mat4));
    scene->meshes[0]->instance_transforms[0] = mat4_identity();

    BuildScene(context, scene);
    SDL_Log("Scene loaded");

    return scene;
}

void SceneLoadMaterialsAndTextures(VulkanContext *context,
                                   Scene *scene,
                                   cgltf_data *data,
                                   const char *directory) {
    // Copy material data
    void *mapped_mat_buffer;
    const u32 offset = scene->materials_count * sizeof(Material);
    MapBuffer(context->device, &scene->mat_buffer, &mapped_mat_buffer);
    GLTFLoadMaterialBuffer(data, (Material *)mapped_mat_buffer);
    UnmapBuffer(context->device, &scene->mat_buffer);

    scene->materials_count += data->materials_count;

    // Textures TODO
    SDL_Log("Loading textures...");
    scene->textures_count += data->textures_count;
    if(data->textures_count > 0) {
        SDL_Surface **surfaces =
            (SDL_Surface **)calloc(data->textures_count, sizeof(SDL_Surface *));
        Buffer *image_buffers = (Buffer *)calloc(data->textures_count, sizeof(Buffer));
        VkCommandBuffer *cmds =
            (VkCommandBuffer *)calloc(data->textures_count, sizeof(VkCommandBuffer));

        AllocateCommandBuffers(
            context->device, context->graphics_command_pool, data->textures_count, cmds);

        for(u32 i = 0; i < data->textures_count; ++i) {
            char *image_path = data->textures[i].image->uri;
            ASSERT_MSG(image_path,
                       "Attempting to load an embedded texture. "
                       "This isn't supported yet");
            u32 file_path_length = strlen(directory) + strlen(image_path) + 1;
            char *full_image_path = (char *)calloc(file_path_length, sizeof(char *));
            strcat_s(full_image_path, file_path_length, directory);
            strcat_s(full_image_path, file_path_length, image_path);

            SDL_Surface *temp_surf = IMG_Load(full_image_path);
            if(!temp_surf) {
                SDL_LogError(0, IMG_GetError());
            }

            if(temp_surf->format->format != SDL_PIXELFORMAT_ABGR8888) {
                surfaces[i] = SDL_ConvertSurfaceFormat(temp_surf, SDL_PIXELFORMAT_ABGR8888, 0);
                if(!surfaces[i]) {
                    SDL_LogError(0, SDL_GetError());
                }
                SDL_FreeSurface(temp_surf);
            } else {
                surfaces[i] = temp_surf;
            }
            free(full_image_path);
        }

        for(u32 i = 0; i < data->textures_count; ++i) {
            VkDeviceSize image_size = surfaces[i]->h * surfaces[i]->pitch;
            VkExtent2D extent = {(u32)surfaces[i]->h, (u32)surfaces[i]->w};

            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
            if(data->textures[i].type == cgltf_texture_type_base_color)
                format = VK_FORMAT_R8G8B8A8_SRGB;

            CreateImage(context->device,
                        &context->memory_properties,
                        format,
                        extent,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        &scene->textures[i]);
            DEBUGNameImage(context->device, &scene->textures[i], data->textures[i].image->uri);
            CreateBuffer(context->device,
                         &context->memory_properties,
                         image_size,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &image_buffers[i]);
            SDL_LockSurface(surfaces[i]);
            UploadToBuffer(context->device, &image_buffers[i], surfaces[i]->pixels, image_size);
            SDL_UnlockSurface(surfaces[i]);
            BeginCommandBuffer(
                context->device, cmds[i], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            CopyBufferToImage(
                cmds[i], extent, surfaces[i]->pitch, &image_buffers[i], &scene->textures[i]);
            vkEndCommandBuffer(cmds[i]);
            VkSubmitInfo si = {
                VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmds[i], 0, NULL};
            vkQueueSubmit(context->graphics_queue, 1, &si, VK_NULL_HANDLE);
        }

        vkQueueWaitIdle(context->graphics_queue);

        vkFreeCommandBuffers(
            context->device, context->graphics_command_pool, data->textures_count, cmds);
        free(cmds);
        for(u32 i = 0; i < data->textures_count; ++i) {
            SDL_FreeSurface(surfaces[i]);
            DestroyBuffer(context->device, &image_buffers[i]);
        }
        free(surfaces);
        free(image_buffers);
    }
}

DLL_EXPORT void VulkanFreeScene(VulkanContext *context, Scene *scene) {
    for(u32 i = 0; i < scene->mesh_count; ++i) {
        FreeMesh(context, scene->meshes[i]);
    }
    free(scene->meshes);

    vkFreeDescriptorSets(context->device,
                         context->descriptor_pool,
                         scene->descriptor_set_count,
                         scene->descriptor_sets);
    free(scene->descriptor_sets);

    for(u32 i = 0; i < scene->descriptor_set_count; ++i) {
        vkDestroyDescriptorSetLayout(context->device, scene->set_layouts[i], 0);
    }
    free(scene->set_layouts);
    vkDestroyPipelineLayout(context->device, scene->layout, 0);

    vkDestroyPipeline(context->device, scene->pipeline, NULL);

    for(u32 i = 0; i < scene->textures_count; ++i) {
        DestroyImage(context->device, &scene->textures[i]);
    }
    free(scene->textures);

    DestroyBuffer(context->device, &scene->mat_buffer);

    free(scene);
}