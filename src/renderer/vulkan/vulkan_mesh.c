
#include "renderer/renderer.h"
#include "renderer/vulkan/vulkan_renderer.h"

void RendererLoadMaterialsAndTextures(Renderer *context, cgltf_data *data, const char *directory) {
    // Copy material data
    void *mapped_mat_buffer;
    //const u32 offset = context->materials_count * sizeof(Material);
    MapBuffer(context->device, &context->mat_buffer, &mapped_mat_buffer);
    GLTFLoadMaterialBuffer(data, (Material *)mapped_mat_buffer);
    UnmapBuffer(context->device, &context->mat_buffer);

    context->materials_count += data->materials_count;

    sLog("Loading textures...");
    u32 texture_start = context->textures_count;
    context->textures_count += data->textures_count;

    if(data->textures_count > 0) {
        // TODO : move that elsewhere, and recreate the pipeline because we have more textures now
        if(context->textures_count > context->textures_capacity) {
            Image *new_buffer =
                (Image *)sRealloc(context->textures, context->textures_count * sizeof(Image));
            ASSERT(new_buffer);
            context->textures = new_buffer;
        }
        Buffer *image_buffers = (Buffer *)sCalloc(data->textures_count, sizeof(Buffer));
        VkCommandBuffer *cmds =
            (VkCommandBuffer *)sCalloc(data->textures_count, sizeof(VkCommandBuffer));

        AllocateCommandBuffers(
            context->device, context->graphics_command_pool, data->textures_count, cmds);

        for(u32 i = 0; i < data->textures_count; ++i) {
            u32 j = texture_start + i;

            char *image_path = data->textures[i].image->uri;
            ASSERT_MSG(image_path,
                       "Attempting to load an embedded texture. "
                       "This isn't supported yet");
            char full_image_path[256] = {0};
            strcat_s(full_image_path, 256, directory);
            strcat_s(full_image_path, 256, image_path);

            u32 w = 0;
            u32 h = 0;
            if(!sQueryImageSize(full_image_path, &w, &h)) {
                sError("Unable to load image %s", image_path);
                continue;
            }

            VkDeviceSize image_size = w * h * 4;
            VkExtent2D extent = {w, h};

            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
            if(data->textures[i].type == cgltf_texture_type_base_color)
                format = VK_FORMAT_R8G8B8A8_SRGB;

            CreateImage(context->device,
                        &context->memory_properties,
                        format,
                        extent,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        &context->textures[j]);
            DEBUGNameImage(context->device, &context->textures[j], data->textures[i].image->uri);
            CreateBuffer(context->device,
                         &context->memory_properties,
                         image_size,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &image_buffers[i]);

            void *dst;
            VkResult result = vkMapMemory(
                context->device, image_buffers[i].memory, 0, image_buffers[i].size, 0, &dst);
            AssertVkResult(result);
            // Load image directly to the buffer
            if(!sLoadImageTo(full_image_path, dst)) {
                sError("Unable to load image %s", image_path);
                continue;
            }
            vkUnmapMemory(context->device, image_buffers[i].memory);

            BeginCommandBuffer(cmds[i], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            CopyBufferToImage(cmds[i], extent, &image_buffers[i], &context->textures[i]);
            vkEndCommandBuffer(cmds[i]);
            VkSubmitInfo si = {
                VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmds[i], 0, NULL};
            vkQueueSubmit(context->graphics_queue, 1, &si, VK_NULL_HANDLE);
        }
        vkQueueWaitIdle(context->graphics_queue);

        // HACK We need a way to update the render group set without recreating it
        DestroyRenderGroup(context, &context->main_render_group);
        CreateMainRenderGroup(context, &context->main_render_group);
        VulkanUpdateTextureDescriptorSet(context->device,
                                         context->main_render_group.descriptor_sets[0],
                                         context->texture_sampler,
                                         context->textures_count,
                                         context->textures);

        vkFreeCommandBuffers(
            context->device, context->graphics_command_pool, data->textures_count, cmds);
        sFree(cmds);
        for(u32 i = 0; i < data->textures_count; ++i) {
            DestroyBuffer(context->device, &image_buffers[i]);
        }
        sFree(image_buffers);
    }
}

u32 RendererLoadMesh(Renderer *renderer, const char *path) {
    sLog("Loading Mesh...");
    Mesh *mesh = (Mesh *)sMalloc(sizeof(Mesh));
    *mesh = (Mesh){0};
    renderer->mesh_count++;

    char directory[64] = {0};
    const char *last_sep = strrchr(path, '/');
    u32 size = last_sep - path;
    strncpy_s(directory, ARRAY_SIZE(directory), path, size);
    directory[size] = '/';
    directory[size + 1] = '\0';

    cgltf_data *data;
    cgltf_options options = {0};
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if(result != cgltf_result_success) {
        sError("Error reading mesh");
        ASSERT(0);
    }

    cgltf_load_buffers(&options, data, path);

    // Transforms
    mesh->primitive_nodes_count = data->nodes_count;
    mesh->primitive_transforms = (Mat4 *)sCalloc(mesh->primitive_nodes_count, sizeof(Mat4));
    GLTFLoadTransforms(data, mesh->primitive_transforms);

    // Primitives
    mesh->total_primitives_count = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        mesh->total_primitives_count += data->meshes[m].primitives_count;
    };
    mesh->primitives = (Primitive *)sCalloc(mesh->total_primitives_count, sizeof(Primitive));

    u32 i = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        for(u32 p = 0; p < data->meshes[m].primitives_count; p++) {
            cgltf_primitive *prim = &data->meshes[m].primitives[p];

            Primitive *primitive = &mesh->primitives[i];

            primitive->vertex_count = (u32)prim->attributes[0].data->count;
            primitive->vertex_offset = mesh->total_vertex_count;
            mesh->total_vertex_count += (u32)prim->attributes[0].data->count;

            primitive->index_count += prim->indices->count;
            primitive->index_offset = mesh->total_index_count;
            mesh->total_index_count += (u32)prim->indices->count;

            primitive->material_id = GLTFGetMaterialID(prim->material) + renderer->materials_count;

            ++i;
        }
    }
    mesh->all_index_offset = mesh->total_vertex_count * sizeof(Vertex);
    const u32 buffer_size =
        mesh->total_vertex_count * sizeof(Vertex) + mesh->total_index_count * sizeof(u32);

    // TODO : Move that into something like this : RendererLoadGLTF(renderer, data);
    {
        // Vertex & Index Buffer
        mesh->buffer = (Buffer *)sMalloc(sizeof(Buffer));
        CreateBuffer(renderer->device,
                     &renderer->memory_properties,
                     buffer_size,
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR |
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     mesh->buffer);
        DEBUGNameBuffer(renderer->device, mesh->buffer, "GLTF VTX/IDX");

        void *mapped_buffer;
        MapBuffer(renderer->device, mesh->buffer, &mapped_buffer);
        i = 0;
        for(u32 m = 0; m < data->meshes_count; ++m) {
            for(u32 p = 0; p < data->meshes[m].primitives_count; ++p) {
                GLTFLoadVertexAndIndexBuffer(&data->meshes[m].primitives[p],
                                             &mesh->primitives[i],
                                             mesh->total_vertex_count * sizeof(Vertex),
                                             mapped_buffer);
                ++i;
            }
        }
        UnmapBuffer(renderer->device, mesh->buffer);

        // Materials
        RendererLoadMaterialsAndTextures(renderer, data, directory);
    }
    mesh->instance_capacity = 1;
    mesh->instance_transforms = (Mat4 *)sCalloc(1, sizeof(Mat4));

    cgltf_free(data);

    // TEMP: hardcoded mesh id
    renderer->meshes[0] = mesh;
    sLog("Loading done");
    return 0;
}

void RendererDestroyMesh(Renderer *renderer, u32 id) {
    Mesh *mesh = renderer->meshes[id];

    sFree(mesh->primitives);

    sFree(mesh->instance_transforms);

    DestroyBuffer(renderer->device, mesh->buffer);
    sFree(mesh->buffer);

    sFree(mesh->primitive_transforms);
    sFree(mesh);
}

void RendererDrawMesh(Frame *frame,
                      Mesh *mesh,
                      const u32 instance_count,
                      const Mat4 *instance_transforms) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(frame->cmd, 0, 1, &mesh->buffer->buffer, &offset);
    vkCmdBindIndexBuffer(
        frame->cmd, mesh->buffer->buffer, mesh->all_index_offset, VK_INDEX_TYPE_UINT32);

    u32 primitive_count = mesh->total_primitives_count;

    for(u32 i = 0; i < instance_count; i++) {
        for(u32 p = 0; p < primitive_count; p++) {
            const Primitive *prim = &mesh->primitives[p];

            Mat4 mat =
                mat4_mul(&instance_transforms[i], &mesh->primitive_transforms[prim->node_id]);
            PushConstant push = {mat, prim->material_id};
            vkCmdPushConstants(frame->cmd,
                               frame->layout,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0,
                               sizeof(PushConstant),
                               &push);
            vkCmdDrawIndexed(
                frame->cmd, prim->index_count, 1, prim->index_offset, prim->vertex_offset, 0);
        }
    }
}

MeshInstance RendererInstantiateMesh(Renderer *renderer, u32 mesh_id) {
    MeshInstance result = {0};

    Mesh *mesh = renderer->meshes[mesh_id];
    if(mesh->instance_count == mesh->instance_capacity) {
        // Resize the buffer
        u32 new_capacity = mesh->instance_capacity * 2;
        sLog("Resizing mesh buffer from %d to %d", mesh->instance_capacity, new_capacity);
        Mat4 *new_buffer = (Mat4 *)sRealloc(mesh->instance_transforms, new_capacity * sizeof(Mat4));
        ASSERT_MSG(new_buffer, "Unable to size up the instance buffer");
        mesh->instance_transforms = new_buffer;
        mesh->instance_capacity = new_capacity;
    }
    u32 instance_id = mesh->instance_count++;
    mesh->instance_transforms[instance_id] = mat4_identity();
    result.transform = &mesh->instance_transforms[instance_id];
    mat4_translate(&mesh->instance_transforms[instance_id],
                   (Vec3){(f32)instance_id * 20.f, 0.f, 0.f});
    return result;
}

void RendererSetCamera(Renderer *renderer, const Vec3 position, const Vec3 forward, const Vec3 up) {
    renderer->camera_info.pos = position;
    renderer->camera_info.view = mat4_look_at(vec3_add(position, forward), position, up);
    mat4_inverse(&renderer->camera_info.view, &renderer->camera_info.view_inverse);
}

void RendererSetSunDirection(Renderer *renderer, const Vec3 direction) {
    const Mat4 a = mat4_ortho_zoom(1.0f / 1.0f, 20.0f, -600.0f, 600.0f);
    Mat4 b = mat4_look_at(
        (Vec3){0.0f, 0.0f, 0.0f}, vec3_fmul(direction, -1.0f), (Vec3){0.0f, 1.0f, 0.0f});

    renderer->camera_info.shadow_mvp = mat4_mul(&a, &b);
    renderer->camera_info.light_dir = direction;
}