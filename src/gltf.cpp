typedef struct Vertex {
    vec3 pos;
    vec3 normal;
} Vertex;

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