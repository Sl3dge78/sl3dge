/*
 === TODO ===
 CRITICAL
 - Materials

 MAJOR
  - Multiple primitives

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
        case cgltf_component_type_r_8u:
        size = sizeof(u8);
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

void GLTFGetNodeTransform(const cgltf_node *node, mat4 *transform) {
    
    if(node->has_matrix) {
        memcpy(transform, node->matrix, sizeof(mat4));
    } else {
        vec3 t = {node->translation[0], node->translation[1], node->translation[2] };
        quat r = {node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3] };
        vec3 s = {node->scale[0], node->scale[1], node->scale[2]};
        
        trs_to_mat4(transform, &t, &r, &s);
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
        
        GLTFGetNodeTransform(node, &transforms[i]);
        
        cgltf_node *parent = node->parent;
        
        while(parent) {
            
            mat4 pm = {};
            GLTFGetNodeTransform(parent, &pm);
            
            for (u32 j = 0; j < 4; ++j)
            {
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
        
        mat4_print(&transforms[i]);
        
        float mat[16];
        cgltf_node_transform_world(node, mat);
        mat4 mama = {};
        memcpy(&mama, mat, sizeof(mat4));
        mat4_print(&mama);
        SDL_Log("==============\n\n");
        
    }
}