

#define PushBufferGetEntry(push_buffer, type) (type *)PushBufferGetEntrySize(push_buffer, sizeof(type))

void *PushBufferGetEntrySize(PushBuffer *push_buffer, u64 size) {
    if(push_buffer->size + size > push_buffer->max_size) {
        u32 new_size = push_buffer->max_size * 2;
        void *new_ptr = sRealloc(push_buffer->buf, new_size);  
        ASSERT(new_ptr);
        push_buffer->buf = new_ptr;
        push_buffer->max_size = new_size;
    }
    void *entry = push_buffer->buf + push_buffer->size;
    push_buffer->size += size;
    return entry;
}

void UIPushQuad(PushBuffer *push_buffer, const u32 x, const u32 y, const u32 w, const u32 h, const Vec4 color) {
    PushBufferEntryUIQuad *entry = PushBufferGetEntry(push_buffer, PushBufferEntryUIQuad);
    entry->type = PushBufferEntryType_UIQuad;
    entry->l = x;
    entry->t = y;
    entry->r = w + x;
    entry->b = h + y;
    entry->colour = color;
}

// TODO(Guigui): Maybe remove the memallocs ??
void UIPushText(PushBuffer *push_buffer, const char *text, const u32 x, const u32 y, const Vec4 color) {
    PushBufferEntryText *entry = PushBufferGetEntry(push_buffer, PushBufferEntryText);
    entry->type = PushBufferEntryType_Text;
    char *saved = sCalloc(128, sizeof(char));
    StringCopyLength(saved, text, 128);
    entry->text = saved;
    entry->x = x;
    entry->y = y;
    entry->colour = color;
}

void UIPushFmt(PushBuffer *push_buffer, const u32 x, const u32 y, const Vec4 color, const char *fmt, ...) {
    PushBufferEntryText *entry = PushBufferGetEntry(push_buffer, PushBufferEntryText);
    entry->type = PushBufferEntryType_Text;
    entry->text = sCalloc(128, sizeof(char));
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(entry->text, 128, fmt, ap);
    va_end(ap);
    entry->x = x;
    entry->y = y;
    entry->colour = color;
}

void UIPushTexture(PushBuffer *push_buffer, const u32 texture, const u32 x, const u32 y, const u32 w, const u32 h) {
    PushBufferEntryTexture *entry = PushBufferGetEntry(push_buffer, PushBufferEntryTexture);
    entry->type = PushBufferEntryType_Texture;
    entry->l = x;
    entry->t = y;
    entry->r = w + x;
    entry->b = h + y;
    entry->texture = texture;
}

/// Adds a mesh to the scene draw calls
/// The transform pointer needs to be alive until drawing happens
void PushMesh(PushBuffer *push_buffer, const MeshHandle mesh, Transform *transform, Vec3 diffuse_color) {
    PushBufferEntryMesh *entry = PushBufferGetEntry(push_buffer, PushBufferEntryMesh);
    entry->type = PushBufferEntryType_Mesh;
    entry->mesh = mesh;
    entry->transform = transform;
    entry->diffuse_color = diffuse_color;
}

void PushSkinnedMesh(PushBuffer *push_buffer, SkinnedMeshHandle skin, Transform *root, Transform *skeleton, Vec3 diffuse_color) {
    PushBufferEntrySkinnedMesh *entry = PushBufferGetEntry(push_buffer, PushBufferEntrySkinnedMesh);
    entry->type = PushBufferEntryType_SkinnedMesh;
    entry->skin = skin;
    entry->transform = root;
    entry->skeleton = skeleton;
    entry->diffuse_color = diffuse_color;
}

/// Adds a bone to the scene draw calls
/// The matrix needs to be local
void DebugPushMatrix(PushBuffer *push_buffer, Mat4 matrix) {
    PushBufferEntryAxisGizmo *entry = PushBufferGetEntry(push_buffer, PushBufferEntryAxisGizmo);
    entry->type = PushBufferEntryType_AxisGizmo;
    entry->line[0] = mat4_mul_vec3(matrix, (Vec3){0.0f, 0.0f, 0.0f});
    entry->line[1] = mat4_mul_vec3(matrix, (Vec3){0.1f, 0.0f, 0.0f});
    entry->line[2] = mat4_mul_vec3(matrix, (Vec3){0.0f, 0.1f, 0.0f});
    entry->line[3] = mat4_mul_vec3(matrix, (Vec3){0.0f, 0.0f, 0.1f});
}

void DebugPushPosition(PushBuffer *push_buffer, Vec3 position) {
    PushBufferEntryAxisGizmo *entry = PushBufferGetEntry(push_buffer, PushBufferEntryAxisGizmo);
    entry->type = PushBufferEntryType_AxisGizmo;
    entry->line[0] = position;
    entry->line[1] = vec3_add(position, VEC3_RIGHT);
    entry->line[2] = vec3_add(position, VEC3_UP);
    entry->line[3] = vec3_add(position, VEC3_FORWARD);
}
