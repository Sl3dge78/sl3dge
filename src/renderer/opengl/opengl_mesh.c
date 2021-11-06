
internal Mesh *RendererGetNewMesh(Renderer *renderer) {
    if(renderer->mesh_count == renderer->mesh_capacity) {
        u32 new_capacity;
        if (renderer->mesh_capacity <= 0) 
            new_capacity = 1;
        else {
            new_capacity = renderer->mesh_capacity * 2;
        }
        void *ptr = sRealloc(renderer->meshes, sizeof(Mesh) * new_capacity);
        ASSERT(ptr);
        if(ptr) {
            renderer->meshes = (Mesh *)ptr;
        }
    }
    renderer->mesh_count ++;
    return &renderer->meshes[renderer->mesh_count-1];
}

internal Skin *RendererGetNewSkin(Renderer *renderer) {
    if(renderer->skin_count == renderer->skin_capacity) {
        u32 new_capacity;
        if (renderer->skin_capacity <= 0) 
            new_capacity = 1;
        else {
            new_capacity = renderer->skin_capacity * 2;
        }
        void *ptr = sRealloc(renderer->skins, sizeof(Skin) * new_capacity);
        ASSERT(ptr);
        if(ptr) {
            renderer->skins = (Skin *)ptr;
        }
    }
    renderer->skin_count ++;
    return &renderer->skins[renderer->skin_count-1];
}

Mesh *RendererLoadMeshFromVertices(Renderer *renderer, const Vertex *vertices, const u32 vertex_count, const u32 *indices, const u32 index_count) {
    sLog("LOAD - Vertices - Vertices: %d, Indices: %d", vertex_count, index_count);
    Mesh *mesh = RendererGetNewMesh(renderer);
    
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

void LoadSkin(Renderer *renderer, Skin *skin, GLTF *gltf) {
    
    ASSERT_MSG(gltf->skin_count == 1, "ASSERT: More than one skin in gltf, this isn't handled yet.");
    GLTFSkin *src_skin = &gltf->skins[0];
    
    skin->joint_count = src_skin->joint_count;
    skin->inverse_bind_matrices = sCalloc(skin->joint_count, sizeof(Mat4));
    GLTFCopyAccessor(gltf, src_skin->inverse_bind_matrices, skin->inverse_bind_matrices, 0, sizeof(Mat4));
    
    skin->joints = RendererAllocateTransforms(renderer, skin->joint_count);
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

void LoadFromGLTF(const char *path, Renderer *renderer, PlatformAPI *platform, Mesh **mesh, Skin **skin, Animation *animation) {
    GLTF *gltf = LoadGLTF(path, platform);
    
    if(mesh != NULL) {
        sLog("LOAD - Mesh - %s", gltf->path);
        *mesh = RendererGetNewMesh(renderer);
        LoadVertexBuffers(*mesh, gltf);
    }
    
    if(skin != NULL) {
        sLog("LOAD - Skin - %s", gltf->path);
        *skin = RendererGetNewSkin(renderer);
        LoadSkin(renderer, *skin, gltf);
    }
    
    if(animation != NULL) {
        sLog("LOAD - Animation - %s", gltf->path);
        LoadAnimation(renderer, animation, gltf);
    }
    
    DestroyGLTF(gltf);
}

void RendererDestroyMesh(Mesh *mesh) {
    glDeleteBuffers(1, &mesh->index_buffer);
    glDeleteBuffers(1, &mesh->vertex_buffer);
}

void RendererDestroySkin(Renderer *renderer, Skin *skin) {
    
    for(u32 i = 0; i < skin->joint_count; i++) {
        if(skin->joint_child_count[i] > 0) {
            sFree(skin->joint_children[i]);
        }
    }
    sFree(skin->joint_parents);
    sFree(skin->joint_children);
    sFree(skin->joint_child_count);
    sFree(skin->global_joint_mats);
    RendererDestroyTransforms(renderer, skin->joint_count, skin->joints);
    sFree(skin->inverse_bind_matrices);
}



// Returns a mesh pointer and keeps ownership
// -------------------------------------------------
#if 0

// Returns a mesh pointer and keeps ownership
internal SkinnedMesh *GetNewSkinnedMesh(Renderer *renderer) {
    u32 new_capacity;
    if(renderer->skinned_mesh_count == renderer->skinned_mesh_capacity) {
        if(renderer->skinned_mesh_capacity <= 0) {
            new_capacity = 1;
        } else {
            new_capacity = renderer->skinned_mesh_capacity * 2;
        }
        void *ptr = sRealloc(renderer->skinned_meshes, sizeof(SkinnedMesh) * new_capacity);
        ASSERT(ptr);
        if(ptr) {
            renderer->skinned_meshes = (SkinnedMesh *)ptr;
        }
    }
    renderer->skinned_mesh_count ++;
    return &renderer->skinned_meshes[renderer->skinned_mesh_count-1];
}

internal void LoadTextures(u32 *dest, cgltf_data *data, const u32 default_texture, const char *directory) {
    // Textures
    if(data->textures_count > 0) {
        for(u32 i = 0; i < data->textures_count; ++i) {
            if(data->textures[i].type == cgltf_texture_type_base_color) {
                char *image_path = data->textures[i].image->uri;
                char full_image_path[256] = {0};
                strcat_s(full_image_path, 256, directory);
                strcat_s(full_image_path, 256, image_path);
                
                PNG_Image *image = sLoadImage(full_image_path);
                u32 texture;
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glObjectLabel(GL_TEXTURE, texture, -1, "NAME");
                glTexImage2D(GL_TEXTURE_2D,
                             0,
                             GL_RGBA,
                             image->width,
                             image->height,
                             0,
                             GL_RGBA,
                             GL_UNSIGNED_BYTE,
                             image->pixels);
                sDestroyImage(image);
                
                *dest = texture;
            }
        }
    } else {
        *dest = default_texture;
    }
}

internal void LoadSkinnedVtxAndIdxBuffers(SkinnedMesh *skinned_mesh, cgltf_data *data) {
    // Vertex & Index Buffer
    Mesh *mesh = &skinned_mesh->mesh;
    glGenBuffers(1, &mesh->vertex_buffer);
    glGenBuffers(1, &mesh->index_buffer);
    glGenVertexArrays(1, &mesh->vertex_array);
    
    glBindVertexArray(mesh->vertex_array);
    u32 i = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        if(data->meshes[m].primitives_count > 1) {
            sWarn("Only 1 primitive supported yet");
            // @TODO
        }
        for(u32 p = 0; p < data->meshes[m].primitives_count; p++) {
            cgltf_primitive *prim = &data->meshes[m].primitives[p];
            mesh->index_count = data->meshes[m].primitives[p].indices->count;
            u32 index_buffer_size = 0;
            void *index_data = GLTFGetIndexBuffer(prim, &index_buffer_size);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer_size, index_data, GL_STATIC_DRAW);
            sFree(index_data);
            
            mesh->vertex_count = (u32)prim->attributes[0].data->count;
            u32 vertex_buffer_size = 0;
            void *vertex_data = GLTFGetSkinnedVertexBuffer(prim, &vertex_buffer_size);
            
            glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
            glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, vertex_data, GL_STATIC_DRAW);
            sFree(vertex_data);
            ++i;
        }
    }
    
    glObjectLabel(GL_BUFFER, mesh->vertex_buffer, -1, "GLTF VTX BUFFER");
    glObjectLabel(GL_BUFFER, mesh->index_buffer, -1, "GLTF IDX BUFFER");
    glObjectLabel(GL_VERTEX_ARRAY, mesh->vertex_array, -1, "GLTF ARRAY BUFFER");
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void *)offsetof(SkinnedVertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
                          1, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void *)offsetof(SkinnedVertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void *)offsetof(SkinnedVertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(SkinnedVertex), (void *)offsetof(SkinnedVertex, joints));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void *)offsetof(SkinnedVertex, weights));
    glEnableVertexAttribArray(4);
}

internal void LoadSkin(Renderer *renderer, SkinnedMesh *mesh, cgltf_data *data) {
    ASSERT_MSG(data->skins_count == 1, "No or multiple skins to load");
    cgltf_skin *skin = data->skins;
    
    ASSERT_MSG(skin->joints_count < 10, "More than 10 bones in mesh, this isn't supported");
    
    for(u32 i = 0; i < skin->joints_count; i++) {
        skin->joints[i]->id = i+1;
    }
    
    mesh->joint_count = skin->joints_count;
    mesh->joints = RendererAllocateTransforms(renderer, skin->joints_count);
    mesh->global_joint_mats = sCalloc(skin->joints_count, sizeof(Mat4));
    mesh->joint_parents = sCalloc(skin->joints_count, sizeof(u32));
    mesh->joint_children = sCalloc(skin->joints_count, sizeof(u32 *));
    mesh->joint_children_count = sCalloc(skin->joints_count, sizeof(u32));
    for(u32 i = 0; i < skin->joints_count; i++) {
        GLTFGetNodeTransform(skin->joints[i], &mesh->joints[i]);
        if(skin->joints[i]->parent && skin->joints[i]->parent->id != 0)
            mesh->joint_parents[i] = skin->joints[i]->parent->id-1;
        else
            mesh->joint_parents[i] = -1;
        
        if(skin->joints[i]->children_count > 0) {
            mesh->joint_children_count[i] = skin->joints[i]->children_count;
            mesh->joint_children[i] = sCalloc(skin->joints[i]->children_count, sizeof(u32));
            for(u32 j = 0; j < skin->joints[i]->children_count; j++) {
                mesh->joint_children[i][j] = skin->joints[i]->children[j]->id;
            }
        }
    }
    ASSERT(skin->inverse_bind_matrices->count > 0);
    mesh->inverse_bind_matrices = sCalloc(skin->inverse_bind_matrices->count, sizeof(Mat4));
    GLTFCopyAccessor(skin->inverse_bind_matrices, mesh->inverse_bind_matrices, 0, sizeof(Mat4));
}

SkinnedMeshHandle RendererLoadSkinnedMesh(Renderer *renderer, const char *path, Animation *animation) {
    sLog("LOAD - SkinnedMesh - %s", path);
    
    SkinnedMesh *mesh = GetNewSkinnedMesh(renderer);
    
    char directory[128] = {0};
    const char *last_sep = strrchr(path, '/');
    u32 size = last_sep - path;
    strncpy_s(directory, ARRAY_SIZE(directory), path, size);
    directory[size] = '/';
    directory[size + 1] = '\0';
    
    cgltf_data *data;
    cgltf_options options = {0};
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if(result != cgltf_result_success) {
        sError("Error reading mesh");
        ASSERT(0);
    }
    
    cgltf_load_buffers(&options, data, path);
    
    LoadSkinnedVtxAndIdxBuffers(mesh, data);
    LoadSkin(renderer, mesh, data);
    LoadTextures(&mesh->mesh.diffuse_texture, data, renderer->white_texture, directory);
    if(data->animations_count > 0) {
        LoadAnimation(&data->animations[0], mesh, animation);
    }
    cgltf_free(data);
    
    return mesh;
}

void RendererDestroySkinnedMesh(Renderer *renderer, SkinnedMesh *mesh) {
    sFree(mesh->inverse_bind_matrices);
    
    for(u32 i = 0; i < mesh->joint_count; i++) {
        if(mesh->joint_children_count[i] > 0) {
            sFree(mesh->joint_children[i]);
        }
    }
    sFree(mesh->joint_children_count);
    sFree(mesh->joint_children);
    sFree(mesh->joint_parents);
    sFree(mesh->global_joint_mats);
    
    RendererDestroyMesh(renderer, &mesh->mesh);
    
}

#endif
