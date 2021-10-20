#include "utils/sl3dge.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "renderer/renderer.h"

internal void GLTFCopyAccessor(cgltf_accessor *acc, void *dst, const u32 offset, const u32 dst_stride) {
    const cgltf_buffer_view *view = acc->buffer_view;
    char *buf = (char *)view->buffer->data + view->offset + acc->offset;
    const u32 stride = acc->stride;
    
    size_t size = 0;
    switch(acc->component_type) {
        case cgltf_component_type_r_16u: size = sizeof(u16); break;
        case cgltf_component_type_r_32u: size = sizeof(u32); break;
        case cgltf_component_type_r_32f: size = sizeof(f32); break;
        case cgltf_component_type_r_8u: size = sizeof(u8); break;
        default:
        sError("Unsupported component type : %d", acc->component_type);
        ASSERT(0);
        break;
    };
    switch(acc->type) {
        case cgltf_type_scalar: break;
        case cgltf_type_vec4: size *= 4; break;
        case cgltf_type_vec3: size *= 3; break;
        case cgltf_type_vec2: size *= 2; break;
        case cgltf_type_mat4: size *= 4 * 4; break;
        default:
        sError("Unsupported type : %d", acc->type);
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

void *GLTFGetIndexBuffer(cgltf_primitive *prim, u32 *size) {
    *size = prim->indices->count * sizeof(u32);
    u32 *result = sCalloc(prim->indices->count, sizeof(u32));
    GLTFCopyAccessor(prim->indices, result, 0, sizeof(u32));
    return result;
}

void *GLTFGetVertexBuffer(cgltf_primitive *prim, u32 *size) {
    Vertex *result = sCalloc(prim->attributes[0].data->count, sizeof(Vertex));
    *size = prim->attributes[0].data->count * sizeof(Vertex);
    for(u32 a = 0; a < prim->attributes_count; ++a) {
        switch(prim->attributes[a].type) {
            case cgltf_attribute_type_position: {
                GLTFCopyAccessor(
                                 prim->attributes[a].data, result, offsetof(Vertex, pos), sizeof(Vertex));
            } break;
            
            case cgltf_attribute_type_normal: {
                GLTFCopyAccessor(
                                 prim->attributes[a].data, result, offsetof(Vertex, normal), sizeof(Vertex));
            } break;
            case cgltf_attribute_type_texcoord: {
                GLTFCopyAccessor(
                                 prim->attributes[a].data, result, offsetof(Vertex, uv), sizeof(Vertex));
            } break;
            default: {
                continue;
            }
        }
    }
    return result;
}

void *GLTFGetSkinnedVertexBuffer(cgltf_primitive *prim, u32 *size) {
    SkinnedVertex *result = sCalloc(prim->attributes[0].data->count, sizeof(SkinnedVertex));
    *size = prim->attributes[0].data->count * sizeof(SkinnedVertex);
    for(u32 a = 0; a < prim->attributes_count; ++a) {
        switch(prim->attributes[a].type) {
            case cgltf_attribute_type_position: {
                GLTFCopyAccessor(
                                 prim->attributes[a].data, result, offsetof(SkinnedVertex, pos), sizeof(SkinnedVertex));
            } break;
            
            case cgltf_attribute_type_normal: {
                GLTFCopyAccessor(
                                 prim->attributes[a].data, result, offsetof(SkinnedVertex, normal), sizeof(SkinnedVertex));
            } break;
            case cgltf_attribute_type_texcoord: {
                GLTFCopyAccessor(
                                 prim->attributes[a].data, result, offsetof(SkinnedVertex, uv), sizeof(SkinnedVertex));
            } break;
            case cgltf_attribute_type_joints: {
                GLTFCopyAccessor(prim->attributes[a].data, result, offsetof(SkinnedVertex, joints), sizeof(SkinnedVertex));
            } break;
            case cgltf_attribute_type_weights: {
                GLTFCopyAccessor(prim->attributes[a].data, result, offsetof(SkinnedVertex, weights), sizeof(SkinnedVertex));
            } break;
            default: {
                continue;
            }
        }
    }
    return result;
}

/* // @TODO this is vulkan specific
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
        Material dst = {0};

        if(!mat->has_pbr_metallic_roughness) {
            // @TODO Handle metallic roughness
            sError("Only metallic_roughness is supported");
            ASSERT(0);
        }

        float *color = mat->pbr_metallic_roughness.base_color_factor;

        if(mat->pbr_metallic_roughness.base_color_texture.has_transform) {
            sWarn("Texture has transform, this isn't supported yet");
        }
        Vec3 base_color = {color[0], color[1], color[2]};
        dst.base_color = base_color;
        dst.base_color_texture =
            GLTFGetTextureID(mat->pbr_metallic_roughness.base_color_texture.texture);
        dst.metallic_roughness_texture =
            GLTFGetTextureID(mat->pbr_metallic_roughness.metallic_roughness_texture.texture);
        dst.metallic_factor = mat->pbr_metallic_roughness.metallic_factor;
        dst.roughness_factor = mat->pbr_metallic_roughness.roughness_factor;
        dst.normal_texture = GLTFGetTextureID(mat->normal_texture.texture);
        dst.ao_texture = GLTFGetTextureID(mat->occlusion_texture.texture);
        dst.emissive_texture = GLTFGetTextureID(mat->emissive_texture.texture);
        // sLog("%f, %f, %f, metallic : %f, roughness : %f", color[0], color[1],color[2],
        // dst.metallic, dst.roughness);

        memcpy(buffer, &dst, sizeof(Material));

        buffer++;
    }
}
*/
void GLTFGetNodeTransform(const cgltf_node *node, Transform *transform) {
    if(node->has_matrix) {
        mat4_to_transform((Mat4 *)node->matrix, transform);
    } else {
        transform->translation = *(Vec3 *)(&node->translation);
        transform->rotation = *(Quat *)(&node->rotation);
        transform->scale = *(Vec3 *)(&node->scale);
    }
}
/*
void GLTFLoadTransforms(cgltf_data *data, Transform *transforms) {
    for(u32 i = 0; i < data->nodes_count; i++) {
        TransformIdentity(&transforms[i]);
        cgltf_node *node = &data->nodes[i];
        
        GLTFGetNodeTransform(node, &transforms[i]);
        
        cgltf_node *parent = node->parent;
        
        while(parent) {
            Transform pm = {0};
            GLTFGetNodeTransform(parent, &pm);
            
            for(u32 j = 0; j < 4; ++j) {
                float l0 = transforms[i][j*4+0];
                float l1 = transforms[i][j*4+1];
                float l2 = transforms[i][j*4+2];
                
                float r0 = l0 * pm[0] + l1 * pm[4] + l2 * pm[8];
                float r1 = l0 * pm[1] + l1 * pm[5] + l2 * pm[9];
                float r2 = l0 * pm[2] + l1 * pm[6] + l2 * pm[10];
                
                transforms[i][j*4+0] = r0;
                transforms[i][j*4+1] = r1;
                transforms[i][j*4+2] = r2;
            }
            
            transforms[i][12] += pm[12];
            transforms[i][13] += pm[13];
            transforms[i][14] += pm[14];
            
            parent = parent->parent;
        }
    }
}
*/