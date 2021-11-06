
Mesh *LoadMeshFromVertices(Renderer *renderer, const Vertex *vertices, const u32 vertex_count, const u32 *indices, const u32 index_count) {
    sLog("LOAD - Vertices - Vertices: %d, Indices: %d", vertex_count, index_count);
    Mesh *mesh = (Mesh *)ArrayGetNewElement(&renderer->meshes);
    
    mesh->index_count = index_count;
    mesh->vertex_count = vertex_count;
    
    // Vertex & Index Buffer
    glGenBuffers(1, &mesh->vertex_buffer);
    glGenBuffers(1, &mesh->index_buffer);
    glGenVertexArrays(1, &mesh->vertex_array);
    glBindVertexArray(mesh->vertex_array);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(u32), indices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
                          1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
    
    glObjectLabel(GL_BUFFER, mesh->vertex_buffer, -1, "GEN VTX BUFFER");
    glObjectLabel(GL_BUFFER, mesh->index_buffer, -1, "GEN IDX BUFFER");
    glObjectLabel(GL_VERTEX_ARRAY, mesh->vertex_array, -1, "GEN ARRAY BUFFER");
    
    return mesh;
}

internal void LoadVertexBuffers(Mesh *mesh, const GLTF *gltf) {
    
    // Vertex & Index Buffer
    glGenBuffers(1, &mesh->vertex_buffer);
    glGenBuffers(1, &mesh->index_buffer);
    glGenVertexArrays(1, &mesh->vertex_array);
    
    glBindVertexArray(mesh->vertex_array);
    for(u32 m = 0; m < gltf->mesh_count; ++m) {
        if(gltf->meshes[m].primitive_count > 1) {
            sWarn("Only 1 primitive supported yet");
            // @TODO
        }
        
        GLTFPrimitive *prim = &gltf->meshes[m].primitives[0];
        
        // Indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer);
        GLTFAccessor *indices_acc = &gltf->accessors[prim->indices];
        u32 index_buffer_size = indices_acc->count * sizeof(u32);
        mesh->index_count = indices_acc->count;
        u32 *index_data = sCalloc(indices_acc->count, sizeof(u32));
        GLTFCopyAccessor(gltf, prim->indices, index_data, 0, sizeof(u32));
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer_size, index_data, GL_STATIC_DRAW);
        sFree(index_data);
        
        // Vertices
        // We're doing some offsets shenanigans, so we need to make sure that the offsets of Vertex are identical in SkinnedVertex
        ASSERT(offsetof(Vertex, pos) == offsetof(SkinnedVertex, pos));
        ASSERT(offsetof(Vertex, uv) == offsetof(SkinnedVertex, uv));
        ASSERT(offsetof(Vertex, normal) == offsetof(SkinnedVertex, normal));
        
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
        mesh->vertex_count = (u32)gltf->accessors[prim->position].count;
        u32 vertex_buffer_size;
        void *vertex_data;
        u32 vertex_size;
        if(prim->attributes_set & PRIMITIVE_SKINNED) {
            vertex_data = sCalloc(mesh->vertex_count, sizeof(SkinnedVertex));
            vertex_buffer_size = mesh->vertex_count * sizeof(SkinnedVertex);
            vertex_size = sizeof(SkinnedVertex);
        } else {
            vertex_data = sCalloc(mesh->vertex_count, sizeof(Vertex));
            vertex_buffer_size =  mesh->vertex_count * sizeof(Vertex);
            vertex_size = sizeof(Vertex);
        }
        
        if(prim->attributes_set & PRIMITIVE_ATTRIBUTE_POSITION) {
            GLTFCopyAccessor(gltf, prim->position, vertex_data, offsetof(SkinnedVertex, pos), vertex_size);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_size, (void *)offsetof(SkinnedVertex, pos));
            glEnableVertexAttribArray(0);
        }
        if(prim->attributes_set & PRIMITIVE_ATTRIBUTE_NORMAL) {
            GLTFCopyAccessor(gltf, prim->normal, vertex_data, offsetof(SkinnedVertex, normal), vertex_size);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_size, (void *)offsetof(SkinnedVertex, normal));
            glEnableVertexAttribArray(1);
        } 
        if(prim->attributes_set & PRIMITIVE_ATTRIBUTE_TEXCOORD_0) {
            GLTFCopyAccessor(gltf, prim->texcoord_0, vertex_data, offsetof(SkinnedVertex, uv), vertex_size);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertex_size, (void *)offsetof(SkinnedVertex, uv));
            glEnableVertexAttribArray(2);
        } 
        if(prim->attributes_set & PRIMITIVE_ATTRIBUTE_JOINTS_0) {
            GLTFCopyAccessor(gltf, prim->joints_0, vertex_data, offsetof(SkinnedVertex, joints), vertex_size);
            glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_FALSE, vertex_size, (void *)offsetof(SkinnedVertex, joints));
            glEnableVertexAttribArray(3);
        } 
        if(prim->attributes_set & PRIMITIVE_ATTRIBUTE_WEIGHTS_0) {
            GLTFCopyAccessor(gltf, prim->weights_0, vertex_data, offsetof(SkinnedVertex, weights), vertex_size);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, vertex_size, (void *)offsetof(SkinnedVertex, weights));
            glEnableVertexAttribArray(4);
        } 
        glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, vertex_data, GL_STATIC_DRAW);
        
        // SkinnedVertex *a = (SkinnedVertex *)vertex_data;
        // glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, (void*)a, GL_STATIC_DRAW);
        
        sFree(vertex_data);
    }
    
    char buf[64] = {0};
    strcat(buf, gltf->path);
    u32 i = strlen(buf);
    strcat(buf, " Vertex buffer");
    glObjectLabel(GL_BUFFER, mesh->vertex_buffer, -1, buf);
    buf[i] = '\0';
    strcat(buf, " Index buffer");
    glObjectLabel(GL_BUFFER, mesh->index_buffer, -1, buf);
    buf[i] = '\0';
    strcat(buf, " Array buffer");
    glObjectLabel(GL_VERTEX_ARRAY, mesh->vertex_array, -1, buf);
    
}

internal void LoadSkin(Renderer *renderer, Skin *skin, GLTF *gltf) {
    
    ASSERT_MSG(gltf->skin_count == 1, "ASSERT: More than one skin in gltf, this isn't handled yet.");
    GLTFSkin *src_skin = &gltf->skins[0];
    
    skin->joint_count = src_skin->joint_count;
    skin->inverse_bind_matrices = sCalloc(skin->joint_count, sizeof(Mat4));
    GLTFCopyAccessor(gltf, src_skin->inverse_bind_matrices, skin->inverse_bind_matrices, 0, sizeof(Mat4));
    
    skin->joints = AllocateTransforms(renderer, skin->joint_count);
    skin->global_joint_mats = sCalloc(skin->joint_count, sizeof(Mat4));
    
    skin->joint_child_count = sCalloc(skin->joint_count, sizeof(u32));
    skin->joint_children = sCalloc(skin->joint_count, sizeof(u32 *));
    
    skin->joint_parents = sCalloc(skin->joint_count, sizeof(u32));
    
    for(u32 i = 0; i < skin->joint_count; ++i) {
        GLTFNode *node = &gltf->nodes[src_skin->joints[i]];
        
        skin->joints[i] = node->xform;
        skin->joint_child_count[i] = node->child_count;
        skin->joint_parents[i] = -1;
        if(node->child_count > 0) {
            skin->joint_children[i] = sCalloc(node->child_count, sizeof(u32));
            for(u32 j = 0; j < skin->joint_child_count[i]; ++j) {
                u32 child_id = GLTFGetBoneIDFromNode(gltf, node->children[j]);
                skin->joint_children[i][j] = child_id;
                skin->joint_parents[child_id] = i; 
            }
        }
    }
}

void LoadFromGLTF(const char *path, Renderer *renderer, PlatformAPI *platform, Mesh **mesh, Skin **skin, Animation **animation) {
    GLTF *gltf = LoadGLTF(path, platform);
    
    if(mesh != NULL) {
        sLog("LOAD - Mesh - %s", gltf->path);
        *mesh = (Mesh *)ArrayGetNewElement(&renderer->meshes);
        LoadVertexBuffers(*mesh, gltf);
    }
    
    if(skin != NULL) {
        sLog("LOAD - Skin - %s", gltf->path);
        *skin = (Skin *)ArrayGetNewElement(&renderer->skins);
        LoadSkin(renderer, *skin, gltf);
    }
    
    if(animation != NULL) {
        sLog("LOAD - Animation - %s", gltf->path);
        *animation = (Animation *)ArrayGetNewElement(&renderer->animations);
        LoadAnimation(*animation, gltf);
    }
    
    DestroyGLTF(gltf);
}

void DestroyMesh(Mesh *mesh) {
    glDeleteBuffers(1, &mesh->index_buffer);
    glDeleteBuffers(1, &mesh->vertex_buffer);
}

void DestroySkin(Renderer *renderer, Skin *skin) {
    
    for(u32 i = 0; i < skin->joint_count; i++) {
        if(skin->joint_child_count[i] > 0) {
            sFree(skin->joint_children[i]);
        }
    }
    sFree(skin->joint_parents);
    sFree(skin->joint_children);
    sFree(skin->joint_child_count);
    sFree(skin->global_joint_mats);
    DestroyTransforms(renderer, skin->joint_count, skin->joints);
    sFree(skin->inverse_bind_matrices);
}

