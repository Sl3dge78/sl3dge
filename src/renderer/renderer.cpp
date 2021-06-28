#include <SDL/SDL.h>
#include <sl3dge/sl3dge.h>

#include "renderer/renderer.h"

//#if defined(RENDERER_VULKAN)
#include "renderer/vulkan/vulkan_renderer.cpp"

//#endif

void RendererLoadMaterialsAndTextures(Renderer *context, cgltf_data *data, const char *directory) {
    // Copy material data
    void *mapped_mat_buffer;
    const u32 offset = context->materials_count * sizeof(Material);
    MapBuffer(context->device, &context->mat_buffer, &mapped_mat_buffer);
    GLTFLoadMaterialBuffer(data, (Material *)mapped_mat_buffer);
    UnmapBuffer(context->device, &context->mat_buffer);

    context->materials_count += data->materials_count;

    SDL_Log("Loading textures...");
    u32 texture_start = context->textures_count;
    context->textures_count += data->textures_count;
    // BUG : Sometimes we get an exception here
    if(data->textures_count > 0) {
        // TODO : move that elsewhere, and recreate the pipeline because we have more textures now
        if(context->textures_count > context->textures_capacity) {
            Image *new_buffer =
                (Image *)srealloc(context->textures, context->textures_count * sizeof(Image));
            ASSERT(new_buffer);
            context->textures = new_buffer;
        }
        SDL_Surface **surfaces =
            (SDL_Surface **)scalloc(data->textures_count, sizeof(SDL_Surface *));
        Buffer *image_buffers = (Buffer *)scalloc(data->textures_count, sizeof(Buffer));
        VkCommandBuffer *cmds =
            (VkCommandBuffer *)scalloc(data->textures_count, sizeof(VkCommandBuffer));

        AllocateCommandBuffers(
            context->device, context->graphics_command_pool, data->textures_count, cmds);

        for(u32 i = 0; i < data->textures_count; ++i) {
            char *image_path = data->textures[i].image->uri;
            ASSERT_MSG(image_path,
                       "Attempting to load an embedded texture. "
                       "This isn't supported yet");
            u32 file_path_length = strlen(directory) + strlen(image_path) + 1;
            char *full_image_path = (char *)scalloc(file_path_length, sizeof(char *));
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
            sfree(full_image_path);
        }

        for(u32 i = 0; i < data->textures_count; ++i) {
            u32 j = texture_start + i;

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
                        &context->textures[j]);
            DEBUGNameImage(context->device, &context->textures[j], data->textures[i].image->uri);
            CreateBuffer(context->device,
                         &context->memory_properties,
                         image_size,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &image_buffers[i]);

            if(SDL_MUSTLOCK(surfaces[i]))
                SDL_LockSurface(surfaces[i]);
            UploadToBuffer(context->device, &image_buffers[i], surfaces[i]->pixels, image_size);
            if(SDL_MUSTLOCK(surfaces[i]))
                SDL_UnlockSurface(surfaces[i]);
            BeginCommandBuffer(
                context->device, cmds[i], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            CopyBufferToImage(
                cmds[i], extent, surfaces[i]->pitch, &image_buffers[i], &context->textures[i]);
            vkEndCommandBuffer(cmds[i]);
            VkSubmitInfo si = {
                VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmds[i], 0, NULL};
            vkQueueSubmit(context->graphics_queue, 1, &si, VK_NULL_HANDLE);
        }
        vkQueueWaitIdle(context->graphics_queue);

        DestroyRenderGroup(context, &context->main_render_group);
        CreateMainRenderGroup(context, &context->main_render_group);
        VulkanUpdateTextureDescriptorSet(context->device,
                                         context->main_render_group.descriptor_sets[0],
                                         context->texture_sampler,
                                         context->textures_count,
                                         context->textures);

        vkFreeCommandBuffers(
            context->device, context->graphics_command_pool, data->textures_count, cmds);
        sfree(cmds);
        for(u32 i = 0; i < data->textures_count; ++i) {
            SDL_FreeSurface(surfaces[i]);
            DestroyBuffer(context->device, &image_buffers[i]);
        }
        sfree(surfaces);
        sfree(image_buffers);
    }
}

u32 RendererLoadMesh(Renderer *renderer, const char *path) {
    Mesh *mesh = (Mesh *)smalloc(sizeof(Mesh));
    *mesh = {};
    renderer->mesh_count++;

    char directory[64] = {};
    const char *last_sep = strrchr(path, '/');
    u32 size = last_sep - path;
    strncpy_s(directory, path, size);
    directory[size] = '/';
    directory[size + 1] = '\0';

    cgltf_data *data;
    cgltf_options options = {};
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if(result != cgltf_result_success) {
        SDL_LogError(0, "Error reading mesh");
        ASSERT(0);
    }

    cgltf_load_buffers(&options, data, path);

    // Transforms
    mesh->primitive_nodes_count = data->nodes_count;
    mesh->primitive_transforms = (Mat4 *)scalloc(mesh->primitive_nodes_count, sizeof(Mat4));
    GLTFLoadTransforms(data, mesh->primitive_transforms);

    // Primitives
    mesh->total_primitives_count = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        mesh->total_primitives_count += data->meshes[m].primitives_count;
    };
    mesh->primitives = (Primitive *)scalloc(mesh->total_primitives_count, sizeof(Primitive));

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
        mesh->buffer = (Buffer *)smalloc(sizeof(Buffer));
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
    mesh->instance_transforms = (Mat4 *)scalloc(1, sizeof(Mat4));

    cgltf_free(data);

    // TEMP: hardcoded mesh id
    renderer->meshes[0] = mesh;
    return 0;
}

void RendererDestroyMesh(Renderer *renderer, u32 id) {
    Mesh *mesh = renderer->meshes[id];

    sfree(mesh->primitives);

    sfree(mesh->instance_transforms);

    DestroyBuffer(renderer->device, mesh->buffer);
    sfree(mesh->buffer);

    sfree(mesh->primitive_transforms);
    sfree(mesh);
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
    MeshInstance result = {};

    Mesh *mesh = renderer->meshes[mesh_id];
    if(mesh->instance_count == mesh->instance_capacity) {
        // Resize the buffer
        u32 new_capacity = mesh->instance_capacity * 2;
        SDL_Log("Resizing mesh buffer from %d to %d", mesh->instance_capacity, new_capacity);
        Mat4 *new_buffer = (Mat4 *)srealloc(mesh->instance_transforms, new_capacity * sizeof(Mat4));
        ASSERT_MSG(new_buffer, "Unable to size up the instance buffer");
        mesh->instance_transforms = new_buffer;
        mesh->instance_capacity = new_capacity;
    }
    u32 instance_id = mesh->instance_count++;
    mesh->instance_transforms[instance_id] = mat4_identity();
    result.transform = &mesh->instance_transforms[instance_id];
    mat4_translate(&mesh->instance_transforms[instance_id],
                   Vec3{(f32)instance_id * 20.f, 0.f, 0.f});
    return result;
}

void RendererSetCamera(Renderer *renderer, const Vec3 position, const Vec3 forward, const Vec3 up) {
    renderer->camera_info.pos = position;
    renderer->camera_info.view = mat4_look_at(position + forward, position, up);
    mat4_inverse(&renderer->camera_info.view, &renderer->camera_info.view_inverse);
}

void RendererSetSunDirection(Renderer *renderer, const Vec3 direction) {
    const Mat4 a = mat4_ortho_zoom(1.0f / 1.0f, 20.0f, -600.0f, 600.0f);
    Mat4 b = mat4_look_at({0.0f, 0.0f, 0.0f}, direction * -1.0f, Vec3{0.0f, 1.0f, 0.0f});

    renderer->camera_info.shadow_mvp = mat4_mul(&a, &b);
    renderer->camera_info.light_dir = direction;
}