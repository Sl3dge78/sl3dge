/*
 === TODO ===
 CRITICAL
 
 MAJOR
  - Multiple meshes
- Multiple primitives
- Materials

 BACKLOG
 

 IMPROVEMENTS
 
*/


typedef struct Vertex {
    vec3 pos;
    vec3 normal;
} Vertex;

typedef struct GLTFSceneInfo {
    u32 vertex_buffer_size;
    u32 index_buffer_size;
    
    u32 total_vertex_count;
    u32 total_index_count;
    
    u32 meshes_count;
    
    u32 *vertex_counts;
    u32 *index_counts;
    
    u32 *vertex_offsets;   
    u32 *index_offsets;    
    
    u32 nodes_count;
    u32 *node_mesh;
    
} GLTFSceneInfo;

internal void GLTFCopyAccessor(cgltf_accessor *acc, void* dst, const u32 offset, const u32 dst_stride) {
    
    const cgltf_buffer_view *view = acc->buffer_view;
    char *buf = (char *)view->buffer->data + view->offset + acc->offset;
    const u32 stride = acc->stride;
    
    size_t size = 0;
    switch (acc->component_type) {
        case cgltf_component_type_r_16u:
        size = sizeof(u16);
        break;
        case cgltf_component_type_r_32u:
        size = sizeof(u32);
        break;
        case cgltf_component_type_r_32f:
        size = sizeof(float);
        break;
        default:
        SDL_LogError(0,"Unsupported component type : %d", acc->component_type);
        ASSERT(0);
        break;
    };
    switch (acc->type) {
        case cgltf_type_scalar:
        break;
        case cgltf_type_vec3:
        size *= 3;
        break;
        default:
        SDL_LogError(0,"Unsupported type : %d", acc->type);
        ASSERT(0);
        break;
    };
    
    dst = (void *) ((char*)dst + offset);
    
    for(u32 i = 0; i < acc->count ; i ++) {
        memcpy(dst, buf, size);
        buf += stride;
        dst = (void *) ((char*)dst + dst_stride);
    }
}

void GLTFLoad(cgltf_data *data, GLTFSceneInfo *scene, mat4 *transforms, void **mapped_vtx_buffer, void **mapped_idx_buffer) {
    
    
    for(u32 m = 0; m < scene->meshes_count; ++m) {
        
        cgltf_primitive *prim = &data->meshes[m].primitives[0];
        
        for(u32 a = 0; a < prim->attributes_count ; ++a) {
            if(prim->attributes[a].type == cgltf_attribute_type_position) {
                
                GLTFCopyAccessor(prim->attributes[a].data, *mapped_vtx_buffer, scene->vertex_offsets[m] * sizeof(Vertex) +  offsetof(Vertex, pos), sizeof(Vertex));
                continue;
            }
            
            if(prim->attributes[a].type == cgltf_attribute_type_normal) {
                GLTFCopyAccessor(prim->attributes[a].data, *mapped_vtx_buffer, scene->vertex_offsets[m] * sizeof(Vertex) + offsetof(Vertex, normal), sizeof(Vertex));
                continue;
            }
        }
        
        GLTFCopyAccessor(prim->indices, *mapped_idx_buffer, scene->index_offsets[m] * sizeof(u32), sizeof(u32));
    }
    
    for(u32 i = 0; i < data->nodes_count; i ++) {
        transforms[i] = mat4_identity();
        cgltf_node *node = &data->nodes[i];
        if(node->has_matrix) {
            SDL_LogWarn(0, "Node has matrix, but it isn't supported yet!");
            // TODO(Guigui): 
            continue;
        } else {
            if(node->has_translation) {
                mat4_translate(&transforms[i], {node->translation[0], node->translation[1], node->translation[2]});
            }
            if(node->has_rotation) {
                SDL_LogWarn(0, "Node has rotation, but it isn't supported yet!");
                // TODO(Guigui): 
            }
            if(node->has_scale) {
                SDL_LogWarn(0, "Node has scale, but it isn't supported yet!");
                // TODO(Guigui): 
            }
        }
        
    }
    
    
}