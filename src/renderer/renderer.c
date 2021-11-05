
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
    renderer->mesh_count = 0;
    renderer->mesh_capacity = 8;
    renderer->meshes = sCalloc(renderer->mesh_capacity, sizeof(Mesh));
    
    renderer->skin_count = 0;
    renderer->skin_capacity = 8;
    renderer->skins = sCalloc(renderer->skin_capacity, sizeof(Skin));
    
    renderer->transform_count = 0;
    renderer->transform_capacity = 16;
    renderer->transforms = sCalloc(renderer->transform_capacity, sizeof(Transform));
    
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
    
    BackendRendererInit(&renderer->backend, platform_api, window);
}

DLL_EXPORT void RendererDestroy(Renderer *renderer) {
    
    BackendRendererDestroy(&renderer->backend);
    
    sFree(renderer->ui_pushbuffer.buf);
    sFree(renderer->scene_pushbuffer.buf);
    sFree(renderer->debug_pushbuffer.buf);
    
    // Meshes
    for(u32 i = 0; i < renderer->mesh_count; i++) {
        RendererDestroyMesh(&renderer->meshes[i]);
    }
    sFree(renderer->meshes);
    
    for(u32 i = 0; i < renderer->skin_count; i++) {
        RendererDestroySkin(renderer, &renderer->skins[i]);
    }
    
    sFree(renderer->skins);
    
    
    sFree(renderer->transforms);
}

MeshHandle RendererLoadQuad(Renderer *renderer) {
    const Vertex vertices[] = {
        {{-.5f, 0.0f, -.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{.5f, 0.0f, -.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-.5f, 0.0f, .5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{.5f, 0.0f, .5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}};
    const u32 indices[] = {0, 1, 2, 1, 2, 3};
    
    return RendererLoadMeshFromVertices(renderer, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
}

MeshHandle RendererLoadCube(Renderer *renderer) {
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
    
    return RendererLoadMeshFromVertices(renderer, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
}

Transform *RendererAllocateTransforms(Renderer *renderer, const u32 count) {
    u32 new_xform_count = count + renderer->transform_count;
    while(renderer->transform_capacity < new_xform_count) {
        renderer->transform_capacity *= 2;
        Transform *ptr = sRealloc(renderer->transforms, renderer->transform_capacity);
        ASSERT(ptr);
        renderer->transforms = ptr;
    }
    
    Transform *result = &renderer->transforms[renderer->transform_count];
    renderer->transform_count = new_xform_count;
    transform_identity(result);
    return (result);
}

void RendererDestroyTransforms(Renderer *renderer, const u32 count, const Transform *transforms) {
    // TODO(Guigui): 
    return;
}

internal void UIPushQuad(Renderer *renderer, const u32 x, const u32 y, const u32 w, const u32 h, const Vec4 color) {
    PushBuffer *push_buffer = &renderer->ui_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntryUIQuad) < push_buffer->max_size);
    PushBufferEntryUIQuad *entry = (PushBufferEntryUIQuad *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_UIQuad;
    entry->l = x;
    entry->t = y;
    entry->r = w + x;
    entry->b = h + y;
    entry->colour = color;
    push_buffer->size += sizeof(PushBufferEntryUIQuad);
}

// TODO(Guigui): Maybe remove the memallocs ??
internal void UIPushText(Renderer *renderer, const char *text, const u32 x, const u32 y, const Vec4 color) {
    PushBuffer *push_buffer = &renderer->ui_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntryText) < push_buffer->max_size);
    PushBufferEntryText *entry = (PushBufferEntryText *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Text;
    char *saved = sCalloc(128, sizeof(char));
    StringCopyLength(saved, text, 128);
    entry->text = saved;
    entry->x = x;
    entry->y = y;
    entry->colour = color;
    push_buffer->size += sizeof(PushBufferEntryText);
}

internal void UIPushFmt(Renderer *renderer, const u32 x, const u32 y, const Vec4 color, const char *fmt, ...) {
    PushBuffer *push_buffer = &renderer->ui_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntryText) < push_buffer->max_size);
    PushBufferEntryText *entry = (PushBufferEntryText *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Text;
    entry->text = sCalloc(128, sizeof(char));
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(entry->text, 128, fmt, ap);
    va_end(ap);
    entry->x = x;
    entry->y = y;
    entry->colour = color;
    push_buffer->size += sizeof(PushBufferEntryText);
}

internal void UIPushTexture(Renderer *renderer, const u32 texture, const u32 x, const u32 y, const u32 w, const u32 h) {
    PushBuffer *push_buffer = &renderer->ui_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntryTexture) < push_buffer->max_size);
    PushBufferEntryTexture *entry = (PushBufferEntryTexture *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Texture;
    entry->l = x;
    entry->t = y;
    entry->r = w + x;
    entry->b = h + y;
    entry->texture = texture;
    push_buffer->size += sizeof(PushBufferEntryTexture);
}

/// Adds a mesh to the scene draw calls
/// The transform pointer needs to be alive until drawing happens
internal void PushMesh(Renderer *renderer, MeshHandle mesh, Transform *transform, Vec3 diffuse_color) {
    PushBuffer *push_buffer = &renderer->scene_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntryMesh) < push_buffer->max_size);
    PushBufferEntryMesh *entry = (PushBufferEntryMesh *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Mesh;
    entry->mesh = mesh;
    entry->transform = transform;
    entry->diffuse_color = diffuse_color;
    
    push_buffer->size += sizeof(PushBufferEntryMesh);
}

internal void PushSkin(Renderer *renderer, MeshHandle mesh, SkinHandle skin, Transform *xform, Vec3 diffuse_color) {
    PushBuffer *push_buffer = &renderer->scene_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntrySkin) < push_buffer->max_size);
    PushBufferEntrySkin *entry = (PushBufferEntrySkin *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Skin;
    entry->mesh = mesh;
    entry->skin = skin;
    entry->transform = xform;
    entry->diffuse_color = diffuse_color;
    
    push_buffer->size += sizeof(PushBufferEntrySkin);
}

/// Adds a bone to the scene draw calls
/// The matrix needs to be local
internal void PushBone(Renderer *renderer, Mat4 bone_matrix) {
    PushBuffer *push_buffer = &renderer->debug_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntryBone) < push_buffer->max_size);
    PushBufferEntryBone *entry = (PushBufferEntryBone *)(push_buffer->buf + push_buffer->size);
    
    entry->type = PushBufferEntryType_Bone;
    entry->line[0] = mat4_mul_vec3(bone_matrix, (Vec3){0.0f, 0.0f, 0.0f});
    entry->line[1] = mat4_mul_vec3(bone_matrix, (Vec3){0.1f, 0.0f, 0.0f});
    entry->line[2] = mat4_mul_vec3(bone_matrix, (Vec3){0.0f, 0.1f, 0.0f});
    entry->line[3] = mat4_mul_vec3(bone_matrix, (Vec3){0.0f, 0.0f, 0.1f});
    
    push_buffer->size += sizeof(PushBufferEntryBone);
    
}