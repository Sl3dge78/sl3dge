

#define GetValue(renderer, value) _Generic((value), \
TransformHandle : ArrayGetElementAt(renderer->transforms, value),\
MeshHandle : ArrayGetElementAt(renderer->meshes, value),\
AnimationHandle : ArrayGetElementAt(renderer->animations, value),\
SkinHandle: ArrayGetElementAt(renderer->skins, value), \
default: assert(0))

void CalcChildXform(Renderer *renderer, u32 joint, Skin *skin) {
    for(u32 i = 0; i < skin->joint_child_count[joint]; i++) {
        u32 child = skin->joint_children[joint][i];
        Mat4 tmp;
        Transform *xform = sArrayGet(renderer->transforms, skin->first_joint + child);
        transform_to_mat4(xform, &tmp);
        mat4_mul(skin->global_joint_mats[joint], tmp, skin->global_joint_mats[child]);
        CalcChildXform(renderer, child, skin);
    }
}

void RendererSetSunDirection(Renderer *renderer, const Vec3 direction, const Vec3 center) {
    Mat4 ortho;
    mat4_ortho_zoom_gl(1.0f, 17.0f, -100.0f, 100.0f, ortho);
    Vec3 eye = vec3_add(center, vec3_fmul(direction, -1.0f));
    Mat4 look;
    mat4_look_at(center, eye, (Vec3){0.0f, 1.0f, 0.0f}, look);
    mat4_mul(ortho, look, renderer->light_matrix);
    //renderer->light_dir = vec3_fmul(direction, -1.0f);
    renderer->light_dir = direction;
    // Uniforms are updated each frame in RendererDrawFrame
}

void RendererSetCamera(Renderer *renderer, const Mat4 view, const Vec3 pos) {
    renderer->camera_pos = pos;
    memcpy(renderer->camera_view, view, 16 * sizeof(f32));
    mat4_inverse(view, renderer->camera_view_inverse);
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
    renderer->meshes     = sArrayCreate(8, sizeof(Mesh));
    renderer->skins      = sArrayCreate(1, sizeof(Skin));
    renderer->transforms = sArrayCreate(100, sizeof(Transform));
    renderer->animations = sArrayCreate(1, sizeof(Animation));
    
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
    renderer->debug_pushbuffer.max_size = sizeof(PushBufferEntryAxisGizmo) * 100;
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
        DestroyMesh((Mesh *)sArrayGet(renderer->meshes, i));
    }
    sArrayDestroy(renderer->meshes);
    
    // Skins
    for(u32 i = 0; i < renderer->skins.count; i++) {
        DestroySkin(renderer, (Skin *)sArrayGet(renderer->skins, i));
    }
    sArrayDestroy(renderer->skins);
    
    // Transforms
    sArrayDestroy(renderer->transforms);
    
    // Animations
    for(u32 i = 0; i < renderer->animations.count; i++) {
        DestroyAnimation((Animation *)sArrayGet(renderer->animations, i));
    }
    sArrayDestroy(renderer->animations);
}

MeshHandle LoadQuad(Renderer *renderer) {
    const Vertex vertices[] = {
        {{.5f, 0.0f, .5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{.5f, 0.0f, -.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-.5f, 0.0f, .5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-.5f, 0.0f, -.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}};
    const u32 indices[] = {0, 1, 2, 3, 2, 1};
    
    return LoadMeshFromVertices(renderer, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
}

MeshHandle LoadCube(Renderer *renderer) {
    const Vertex vertices[] = {
        {{-.5f, 0.0f, -.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // +z  //  0
        {{ .5f, 0.0f, -.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},        //  1
        {{-.5f, 1.0f, -.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},        //  2
        {{ .5f, 1.0f, -.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},        //  3
        
        {{-.5f, 0.0f, .5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}}, // -z  //  4
        {{ .5f, 0.0f, .5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},        //  5
        {{-.5f, 1.0f, .5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},        //  6
        {{ .5f, 1.0f, .5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},        //  7
        
        {{-.5f, 1.0f, -.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}, // +y  //  8
        {{ .5f, 1.0f, -.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},        //  9
        {{-.5f, 1.0f, .5f}, {0.0f, 1.0f, -0.0f}, {0.0f, 1.0f}},        // 10
        {{ .5f, 1.0f, .5f}, {0.0f, 1.0f, -0.0f}, {1.0f, 1.0f}},        // 11 

        {{-.5f, 0.0f, -.5f}, {0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}}, //-y // 12
        {{ .5f, 0.0f, -.5f}, {0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},      // 13
        {{-.5f, 0.0f,  .5f}, {0.0f, -1.0f, -0.0f}, {0.0f, 1.0f}},      // 14
        {{ .5f, 0.0f,  .5f}, {0.0f, -1.0f, -0.0f}, {1.0f, 1.0f}},      // 15

        {{.5f, 0.0f, -.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // +x   // 16
        {{.5f, 1.0f, -.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},         // 17
        {{.5f, 0.0f,  .5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},         // 18
        {{.5f, 1.0f,  .5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},         // 19

        {{-.5f, 0.0f, -.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}, // -x // 20
        {{-.5f, 1.0f, -.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},       // 21
        {{-.5f, 0.0f,  .5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},       // 22
        {{-.5f, 1.0f,  .5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},       // 23
    };
    const u32 indices[] = {
        2, 1, 0, 2, 3, 1, // +z
        4, 5, 6, 5, 7, 6, // -z
        10, 9, 8, 11, 9, 10, // +y 
        12, 13, 14, 13, 15, 14, // -y
        16, 17, 18, 17, 19, 18, // +x
        22, 21, 20, 22, 23, 21};// -x
    
    return LoadMeshFromVertices(renderer, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
}

TransformHandle AllocateTransforms(Renderer *renderer, const u32 count) {
    TransformHandle result = (TransformHandle)sArrayAddMultiple(&renderer->transforms, count);
    for(u32 i = 0; i < count; i ++) {
        Transform *x = sArrayGet(renderer->transforms, result + i);
        transform_identity(x);
    }
    return result;
}

void DestroyTransforms(Renderer *renderer, const u32 count, const TransformHandle transforms) {
    // TODO(Guigui): 
    return;
}
