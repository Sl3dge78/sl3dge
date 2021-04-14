typedef struct Vertex {
    vec3 pos;
    vec3 normal;
} Vertex;

typedef struct GLTFAsset {
    u32 size;
    u32 vertex_count;
    u32 index_count;
    u32 vertex_offset;
    u32 index_offset;
    
} GLTFAsset;

void GLTFCopyAccessor(cgltf_accessor *acc, void* dst, const u32 offset, const u32 dst_stride) {
    
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

void GLTFGetAssetInfo(cgltf_data *data, GLTFAsset *asset) {
    cgltf_primitive *prim = &data->meshes[0].primitives[0];
    
    asset->vertex_count = (u32) prim->attributes[0].data->count;
    asset->index_count = prim->indices->count;
    
    asset->size = asset->vertex_count * sizeof(Vertex) + asset->index_count * sizeof(u32);
    
    asset->vertex_offset = 0;
    asset->index_offset = asset->vertex_count * sizeof(Vertex);
}

void GLTFOpen(const char* file, cgltf_data **data) {
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
}

void GLTFLoad(cgltf_data *data, GLTFAsset *asset, void **mapped_buffer) {
    
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
    
    GLTFCopyAccessor(prim->indices, *mapped_buffer, asset->index_offset, sizeof(u32));
    
    
    
    /*
    u32 vtx_count = 0;
    GLTFGetVertexArray(renderer->gltf_data, &vtx_count, NULL);
    Vertex *array = (Vertex *)calloc(vtx_count, sizeof(Vertex));
    GLTFGetVertexArray(renderer->gltf_data, &vtx_count, array);
    
    CreateBuffer(context, vtx_count * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &renderer->gltf_buffer);
    UploadToBuffer(context->device, &renderer->gltf_buffer, array, vtx_count * sizeof(Vertex));
    
    
    u32 idx_count = 0;
    GLTFGetIndexArray(renderer->gltf_data, &idx_count, NULL);
    u32 *idx_array = (u32 *)calloc(idx_count, sizeof(u32));
    GLTFGetIndexArray(renderer->gltf_data, &idx_count, idx_array);
    
    CreateBuffer(context, idx_count * sizeof(u32), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &renderer->gltf_idx_buffer);
    UploadToBuffer(context->device, &renderer->gltf_idx_buffer, idx_array, idx_count * sizeof(u32));
    DEBUGNameBuffer(context->device, &renderer->gltf_idx_buffer, "GLTF idx");
    
    free(array);
    free(idx_array);
*/
    
}

void GLTFGetVertexArray(const cgltf_data *data, u32 *vertex_count, Vertex *array) {
    cgltf_primitive *prim = &data->meshes[0].primitives[0];
    
    if(array == NULL) {
        *vertex_count = (u32) prim->attributes[0].data->count;
        return;
    } 
    
    for(u32 i = 0; i < prim->attributes_count ; ++i) {
        if(prim->attributes[i].type == cgltf_attribute_type_position) {
            
            GLTFCopyAccessor(prim->attributes[i].data, array, offsetof(Vertex, pos), sizeof(Vertex));
            continue;
        }
        
        if(prim->attributes[i].type == cgltf_attribute_type_normal) {
            GLTFCopyAccessor(prim->attributes[i].data, array, offsetof(Vertex, normal), sizeof(Vertex));
            continue;
        }
    }
}

void GLTFGetIndexArray(const cgltf_data *data, u32 *index_count, u32 *array) {
    cgltf_accessor *acc = data->meshes[0].primitives[0].indices;
    if(*index_count == 0) {
        *index_count = acc->count;
        return;
    }
    GLTFCopyAccessor(acc, array, 0, sizeof(u32));
    
}