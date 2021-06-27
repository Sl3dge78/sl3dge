#include <sl3dge/debug.h>
#include "renderer/renderer.h"

internal void
GLTFCopyAccessor(cgltf_accessor *acc, void *dst, const u32 offset, const u32 dst_stride) {
    const cgltf_buffer_view *view = acc->buffer_view;
    char *buf = (char *)view->buffer->data + view->offset + acc->offset;
    const u32 stride = acc->stride;

    size_t size = 0;
    switch(acc->component_type) {
    case cgltf_component_type_r_16u: size = sizeof(u16); break;
    case cgltf_component_type_r_32u: size = sizeof(u32); break;
    case cgltf_component_type_r_32f: size = sizeof(float); break;
    case cgltf_component_type_r_8u: size = sizeof(u8); break;
    default:
        SDL_LogError(0, "Unsupported component type : %d", acc->component_type);
        ASSERT(0);
        break;
    };
    switch(acc->type) {
    case cgltf_type_scalar: break;
    case cgltf_type_vec3: size *= 3; break;
    case cgltf_type_vec2: size *= 2; break;
    default:
        SDL_LogError(0, "Unsupported type : %d", acc->type);
        ASSERT(0);
        break;
    };

    dst = (void *)((char *)dst + offset);

    for(u32 i = 0; i < acc->count; i++) {
        memcpy(dst, buf, size);
        buf += stride;
        dst = (void *)((char *)dst + dst_stride);
    }
}

inline u32 GLTFGetMeshID(cgltf_mesh *mesh) {
    if(!mesh) {
        return UINT_MAX;
    }
    return mesh->id;
}

inline u32 GLTFGetMaterialID(cgltf_material *mat) {
    if(!mat) {
        return UINT_MAX;
    }
    return mat->id;
}

inline u32 GLTFGetTextureID(cgltf_texture *texture) {
    if(!texture)
        return UINT_MAX;

    return texture->id;
}

void GLTFLoadVertexAndIndexBuffer(cgltf_primitive *prim,
                                  Primitive *primitive,
                                  const u32 offset,
                                  void *buffer) {
    GLTFCopyAccessor(
        prim->indices, buffer, offset + primitive->index_offset * sizeof(u32), sizeof(u32));

    for(u32 a = 0; a < prim->attributes_count; ++a) {
        switch(prim->attributes[a].type) {
        case cgltf_attribute_type_position: {
            GLTFCopyAccessor(prim->attributes[a].data,
                             buffer,
                             primitive->vertex_offset * sizeof(Vertex) + offsetof(Vertex, pos),
                             sizeof(Vertex));
        } break;

        case cgltf_attribute_type_normal: {
            GLTFCopyAccessor(prim->attributes[a].data,
                             buffer,
                             primitive->vertex_offset * sizeof(Vertex) + offsetof(Vertex, normal),
                             sizeof(Vertex));
        } break;
        case cgltf_attribute_type_texcoord: {
            GLTFCopyAccessor(prim->attributes[a].data,
                             buffer,
                             primitive->vertex_offset * sizeof(Vertex) + offsetof(Vertex, uv),
                             sizeof(Vertex));
        } break;
        default: {
            continue;
        }
        }
    }
}

void GLTFLoadMaterialBuffer(cgltf_data *data, Material *buffer) {
    for(u32 i = 0; i < data->materials_count; ++i) {
        cgltf_material *mat = &data->materials[i];
        Material dst = {};

        if(!mat->has_pbr_metallic_roughness) {
            // TODO Handle metallic roughness
            SDL_LogError(0, "Only metallic_roughness is supported");
            ASSERT(0);
        }

        float *color = mat->pbr_metallic_roughness.base_color_factor;

        if(mat->pbr_metallic_roughness.base_color_texture.has_transform) {
            SDL_LogWarn(0, "Texture has transform, this isn't supported yet");
        }
        dst.base_color = {color[0], color[1], color[2]};
        dst.base_color_texture =
            GLTFGetTextureID(mat->pbr_metallic_roughness.base_color_texture.texture);
        dst.metallic_roughness_texture =
            GLTFGetTextureID(mat->pbr_metallic_roughness.metallic_roughness_texture.texture);
        dst.metallic_factor = mat->pbr_metallic_roughness.metallic_factor;
        dst.roughness_factor = mat->pbr_metallic_roughness.roughness_factor;
        dst.normal_texture = GLTFGetTextureID(mat->normal_texture.texture);
        dst.ao_texture = GLTFGetTextureID(mat->occlusion_texture.texture);
        dst.emissive_texture = GLTFGetTextureID(mat->emissive_texture.texture);
        // SDL_Log("%f, %f, %f, metallic : %f, roughness : %f", color[0], color[1],color[2],
        // dst.metallic, dst.roughness);

        memcpy(buffer, &dst, sizeof(Material));

        buffer++;
    }
}

void GLTFGetNodeTransform(const cgltf_node *node, Mat4 *transform) {
    if(node->has_matrix) {
        memcpy(transform, node->matrix, sizeof(Mat4));
    } else {
        Vec3 t = {node->translation[0], node->translation[1], node->translation[2]};
        Quat r = {node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]};
        Vec3 s = {node->scale[0], node->scale[1], node->scale[2]};

        trs_to_mat4(transform, &t, &r, &s);
    }
}

void GLTFLoadTransforms(cgltf_data *data, Mat4 *transforms) {
    for(u32 i = 0; i < data->nodes_count; i++) {
        transforms[i] = mat4_identity();
        cgltf_node *node = &data->nodes[i];

        GLTFGetNodeTransform(node, &transforms[i]);

        cgltf_node *parent = node->parent;

        while(parent) {
            Mat4 pm = {};
            GLTFGetNodeTransform(parent, &pm);

            for(u32 j = 0; j < 4; ++j) {
                float l0 = transforms[i].m[j][0];
                float l1 = transforms[i].m[j][1];
                float l2 = transforms[i].m[j][2];

                float r0 = l0 * pm.v[0] + l1 * pm.v[4] + l2 * pm.v[8];
                float r1 = l0 * pm.v[1] + l1 * pm.v[5] + l2 * pm.v[9];
                float r2 = l0 * pm.v[2] + l1 * pm.v[6] + l2 * pm.v[10];

                transforms[i].m[j][0] = r0;
                transforms[i].m[j][1] = r1;
                transforms[i].m[j][2] = r2;
            }

            transforms[i].m[3][0] += pm.m[3][0];
            transforms[i].m[3][1] += pm.m[3][1];
            transforms[i].m[3][2] += pm.m[3][2];

            parent = parent->parent;
        }
    }
}