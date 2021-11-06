
void CalcChildXform(u32 joint, Skin *skin) {
    for(u32 i = 0; i < skin->joint_child_count[joint]; i++) {
        u32 child = skin->joint_children[joint][i];
        Mat4 tmp;
        transform_to_mat4(&skin->joints[child], &tmp);
        mat4_mul(skin->global_joint_mats[joint], tmp, skin->global_joint_mats[child]);
        CalcChildXform(child, skin);
    }
}

void RendererSetSunDirection(Renderer *renderer, const Vec3 direction) {
    Mat4 ortho;
    mat4_ortho_zoom_gl(1.0f, 10.0f, -10.0f, 10.0f, ortho);
    Mat4 look;
    mat4_look_at(vec3_add(direction, renderer->camera_pos),
                 renderer->camera_pos,
                 (Vec3){0.0f, 1.0f, 0.0f}, look);
    mat4_mul(ortho, look, renderer->light_matrix);
    renderer->light_dir = direction;
    // Uniforms are updated each frame in RendererDrawFrame
}

void RendererSetCamera(Renderer *renderer, const Mat4 view, const Vec3 pos) {
    renderer->camera_pos = pos;
    memcpy(renderer->camera_view, view, 16 * sizeof(f32));
    mat4_inverse(view, renderer->camera_view_inverse);
    
    //nocheckin
}

DLL_EXPORT u32 GetRendererSize() {
    u32 result = sizeof(Renderer);
    return result;
}

void UpdateCameraProj(Renderer *renderer) {
    mat4_perspective_gl(90.0f, (f32)renderer->width / (f32)renderer->height, 0.1f, 100000.0f, renderer->camera_proj);
    mat4_inverse(renderer->camera_proj, renderer->camera_proj_inverse);
}

DLL_EXPORT void RendererInit(Renderer *renderer, PlatformAPI *platform_api, PlatformWindow *window) {
    renderer->window = window;
    
    // Arrays
    renderer->meshes     = ArrayCreate(8, sizeof(Mesh));
    renderer->skins      = ArrayCreate(1, sizeof(Skin));
    renderer->transforms = ArrayCreate(8, sizeof(Transform));
    renderer->animations = ArrayCreate(1, sizeof(Animation));
    
    // Init push buffers
    renderer->ui_pushbuffer.size = 0;
    renderer->ui_pushbuffer.max_size = sizeof(PushBufferEntryText) * 256;
    renderer->ui_pushbuffer.buf = sCalloc(renderer->ui_pushbuffer.max_size, 1);
    
    // Scene
    renderer->scene_pushbuffer.size = 0;
    renderer->scene_pushbuffer.max_size = sizeof(PushBufferEntryMesh) * 70;
    renderer->scene_pushbuffer.buf = sCalloc(renderer->scene_pushbuffer.max_size, 1);
    
    // Debug 
    renderer->debug_pushbuffer.size = 0;
    renderer->debug_pushbuffer.max_size = sizeof(PushBufferEntryBone) * 20;
    renderer->debug_pushbuffer.buf = sCalloc(renderer->debug_pushbuffer.max_size, 1);
    
    UpdateCameraProj(renderer);
    
    renderer->backend = sCalloc(1, sizeof(RendererBackend));
    BackendRendererInit(renderer->backend, platform_api, window);
}

DLL_EXPORT void RendererDestroyBackend(Renderer *renderer) {
    BackendRendererDestroy(renderer->backend);
}
DLL_EXPORT void RendererInitBackend(Renderer *renderer, PlatformAPI *platform_api) {
    BackendRendererInit(renderer->backend, platform_api, renderer->window);
}

DLL_EXPORT void RendererDestroy(Renderer *renderer) {
    
    BackendRendererDestroy(renderer->backend);
    sFree(renderer->backend);
    
    sFree(renderer->ui_pushbuffer.buf);
    sFree(renderer->scene_pushbuffer.buf);
    sFree(renderer->debug_pushbuffer.buf);
    
    // Meshes
    for(u32 i = 0; i < renderer->meshes.count; i++) {
        DestroyMesh((Mesh *)ArrayGetElementAt(renderer->meshes, i));
    }
    ArrayDestroy(renderer->meshes);
    
    // Skins
    for(u32 i = 0; i < renderer->skins.count; i++) {
        DestroySkin(renderer, (Skin *)ArrayGetElementAt(renderer->skins, i));
    }
    ArrayDestroy(renderer->skins);
    
    // Transforms
    ArrayDestroy(renderer->transforms);
    
    // Animations
    for(u32 i = 0; i < renderer->animations.count; i++) {
        DestroyAnimation((Animation *)ArrayGetElementAt(renderer->animations, i));
    }
    ArrayDestroy(renderer->animations);
}

Mesh *LoadQuad(Renderer *renderer) {
    const Vertex vertices[] = {
        {{-.5f, 0.0f, -.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{.5f, 0.0f, -.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-.5f, 0.0f, .5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{.5f, 0.0f, .5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}};
    const u32 indices[] = {0, 1, 2, 1, 2, 3};
    
    return LoadMeshFromVertices(renderer, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
}

Mesh *LoadCube(Renderer *renderer) {
    const Vertex vertices[] = {
        {{-.5f, 0.0f, -.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{.5f, 0.0f, -.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-.5f, 1.0f, -.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{.5f, 1.0f, -.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        
        {{-.5f, 0.0f, .5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{.5f, 0.0f, .5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{-.5f, 1.0f, .5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
        {{.5f, 1.0f, .5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        
    };
    const u32 indices[] = {0, 1, 2, 1, 3, 2, 0, 4, 1, 4, 5, 1, 4, 2, 6, 4, 0, 2, 5, 6, 7, 5, 4, 6, 1, 7, 3, 1, 5, 7, 2, 3, 6, 7, 6, 3};
    
    return LoadMeshFromVertices(renderer, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
}

Transform *AllocateTransforms(Renderer *renderer, const u32 count) {
    Transform *result = (Transform *)ArrayGetNewElements(&renderer->transforms, count);
    for(u32 i = 0; i < count; i ++) {
        transform_identity(&result[i]);
    }
    return result;
}

void DestroyTransforms(Renderer *renderer, const u32 count, const Transform *transforms) {
    // TODO(Guigui): 
    return;
}
