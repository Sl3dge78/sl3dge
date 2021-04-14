typedef struct Vertex {
    vec3 pos;
    vec3 normal;
} Vertex;

typedef struct GLTFSceneInfo {
    u32 buffer_size;
    u32 vertex_count;
    u32 index_count;
    u32 vertex_offset;
    u32 index_offset;
    u32 nodes_count;
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

void GLTFOpen(const char* file, cgltf_data **data, GLTFSceneInfo *asset) {
    cgltf_options options = {0};
    cgltf_result result = cgltf_parse_file(&options, file, data);
    if(result != cgltf_result_success) {
        SDL_LogError(0, "Error reading scene");
        ASSERT(0);
    }
    cgltf_load_buffers(&options, *data, file);
    
    if((*data)->meshes_count > 1) {
        SDL_LogError(0, "Multiple meshes not supported, yet");
        ASSERT(0);
    }
    
    cgltf_primitive *prim = &(*data)->meshes[0].primitives[0];
    
    asset->vertex_count = (u32) prim->attributes[0].data->count;
    asset->index_count = prim->indices->count;
    
    asset->buffer_size = asset->vertex_count * sizeof(Vertex) + asset->index_count * sizeof(u32);
    
    asset->vertex_offset = 0;
    asset->index_offset = asset->vertex_count * sizeof(Vertex);
    asset->nodes_count = (*data)->nodes_count;
}

void GLTFClose(cgltf_data *data) {
    cgltf_free(data);
}

void GLTFLoad(cgltf_data *data, GLTFSceneInfo *asset, mat4 *transforms, void **mapped_buffer) {
    
    cgltf_primitive *prim = &data->meshes[0].primitives[0];
    
    for(u32 i = 0; i < prim->attributes_count ; ++i) {
        if(prim->attributes[i].type == cgltf_attribute_type_position) {
            
            GLTFCopyAccessor(prim->attributes[i].data, *mapped_buffer, asset->vertex_offset + offsetof(Vertex, pos), sizeof(Vertex));
            continue;
        }
        
        if(prim->attributes[i].type == cgltf_attribute_type_normal) {
            GLTFCopyAccessor(prim->attributes[i].data, *mapped_buffer, asset->vertex_offset + offsetof(Vertex, normal), sizeof(Vertex));
            continue;
        }
    }
    
    for(u32 i = 0; i < data->nodes_count; i ++) {
        transforms[i] = mat4_identity();
        cgltf_node *node = &data->nodes[i];
        if(node->has_matrix) {
            ASSERT(0);
            // TODO(Guigui): 
            continue;
        } else {
            if(node->has_translation) {
                mat4_translate(&transforms[i], {node->translation[0], node->translation[1], node->translation[2]});
            }
            if(node->has_rotation) {
                ASSERT(0);
                // TODO(Guigui): 
            }
            if(node->has_scale) {
                ASSERT(0);
                // TODO(Guigui): 
            }
        }
        
    }
    
    GLTFCopyAccessor(prim->indices, *mapped_buffer, asset->index_offset, sizeof(u32));
}