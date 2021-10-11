

// Returns a mesh pointer and keeps ownership
internal Mesh *GetNewMesh(Renderer *renderer) {
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

internal void LoadVtxAndIdxBuffers(Mesh *mesh, cgltf_data *data) {
    // Vertex & Index Buffer
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
            void *vertex_data = GLTFGetVertexBuffer(prim, &vertex_buffer_size);
            
            glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
            glBufferData(GL_ARRAY_BUFFER, vertex_buffer_size, vertex_data, GL_STATIC_DRAW);
            sFree(vertex_data);
            ++i;
        }
    }
    
    glObjectLabel(GL_BUFFER, mesh->vertex_buffer, -1, "GLTF VTX BUFFER");
    glObjectLabel(GL_BUFFER, mesh->index_buffer, -1, "GLTF IDX BUFFER");
    glObjectLabel(GL_VERTEX_ARRAY, mesh->vertex_array, -1, "GLTF ARRAY BUFFER");
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
                          1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
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

MeshHandle RendererLoadMesh(Renderer *renderer, const char *path) {
    sLog("LOAD - Mesh - %s", path);
    
    Mesh *mesh = GetNewMesh(renderer);
    
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
    
    LoadVtxAndIdxBuffers(mesh, data);
    LoadTextures(&mesh->diffuse_texture, data, renderer->white_texture, directory);
    
    cgltf_free(data);
    
    return renderer->mesh_count-1;
}

MeshHandle RendererLoadMeshFromVertices(Renderer *renderer, const Vertex *vertices, const u32 vertex_count, const u32 *indices, const u32 index_count) {
    sLog("LOAD - Vertices - Vertices: %d, Indices: %d", vertex_count, index_count);
    Mesh *mesh = GetNewMesh(renderer);
    
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
    
    mesh->diffuse_texture = renderer->white_texture;
    
    return renderer->mesh_count-1;
}

SkinnedMeshHandle RendererLoadSkinnedMesh(Renderer *renderer, const char *path) {
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
    
    cgltf_free(data);
    
    return mesh;
}

void RendererDestroyMesh(Renderer *renderer, Mesh *mesh) {
    glDeleteBuffers(1, &mesh->index_buffer);
    glDeleteBuffers(1, &mesh->vertex_buffer);
    if(mesh->diffuse_texture != renderer->white_texture)
        glDeleteTextures(1, &mesh->diffuse_texture);
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
