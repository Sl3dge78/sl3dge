#include <vulkan/vulkan.h>
#include <SDL/SDL.h>

#include <sl3dge/sl3dge.h>

Mesh *LoadMesh(const char *file, VulkanContext *context, Scene *scene) {
    Mesh *mesh = (Mesh *)malloc(sizeof(Mesh));
    *mesh = {};

    char *directory;
    const char *last_sep = strrchr(file, '/');
    u32 size = last_sep - file;
    directory = (char *)calloc(size + 2, sizeof(char));
    strncpy_s(directory, size + 2, file, size);
    directory[size] = '/';
    directory[size + 1] = '\0';

    cgltf_data *data;
    cgltf_options options = {};
    cgltf_result result = cgltf_parse_file(&options, file, &data);
    if(result != cgltf_result_success) {
        SDL_LogError(0, "Error reading mesh");
        ASSERT(0);
    }

    cgltf_load_buffers(&options, data, file);

    // Transforms
    mesh->primitive_nodes_count = data->nodes_count;
    mesh->primitive_transforms = (Mat4 *)calloc(mesh->primitive_nodes_count, sizeof(Mat4));
    GLTFLoadTransforms(data, mesh->primitive_transforms);

    // Primitives
    mesh->total_primitives_count = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        mesh->total_primitives_count += data->meshes[m].primitives_count;
    };
    mesh->primitives = (Primitive *)calloc(mesh->total_primitives_count, sizeof(Primitive));

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

            primitive->material_id = GLTFGetMaterialID(prim->material) + scene->materials_count;

            ++i;
        }
    }
    mesh->all_index_offset = mesh->total_vertex_count * sizeof(Vertex);
    const u32 buffer_size =
        mesh->total_vertex_count * sizeof(Vertex) + mesh->total_index_count * sizeof(u32);

    // Vertex & Index Buffer
    CreateBuffer(context->device,
                 &context->memory_properties,
                 buffer_size,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &mesh->buffer);
    DEBUGNameBuffer(context->device, &mesh->buffer, "GLTF VTX/IDX");

    void *mapped_buffer;
    MapBuffer(context->device, &mesh->buffer, &mapped_buffer);
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
    UnmapBuffer(context->device, &mesh->buffer);

    // Materials
    SceneLoadMaterialsAndTextures(context, scene, data, directory);

    cgltf_free(data);
    free(directory);

    return mesh;
}

void FreeMesh(VulkanContext *context, Mesh *mesh) {
    free(mesh->primitives);

    free(mesh->instance_transforms);

    DestroyBuffer(context->device, &mesh->buffer);

    free(mesh->primitive_transforms);
    free(mesh);
}

inline void InstanceDraw(VkCommandBuffer cmd,
                         VkPipelineLayout layout,
                         const u32 primitives_count,
                         Primitive *primitives,
                         const Mat4 *primitives_transforms,
                         const Mat4 *mesh_transform) {
    for(u32 p = 0; p < primitives_count; p++) {
        const Primitive *prim = &primitives[p];

        Mat4 mat = mat4_mul(mesh_transform, &primitives_transforms[prim->node_id]);
        PushConstant push = {mat, prim->material_id};
        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &push);
        vkCmdDrawIndexed(cmd, prim->index_count, 1, prim->index_offset, prim->vertex_offset, 0);
    }
}

void MeshDraw(VkCommandBuffer cmd, VkPipelineLayout layout, Mesh *mesh) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->buffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, mesh->buffer.buffer, mesh->all_index_offset, VK_INDEX_TYPE_UINT32);

    for(u32 i = 0; i < mesh->instance_count; i++) {
        InstanceDraw(cmd,
                     layout,
                     mesh->total_primitives_count,
                     mesh->primitives,
                     mesh->primitive_transforms,
                     &mesh->instance_transforms[i]);
    }
}