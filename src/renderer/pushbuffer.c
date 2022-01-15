
 void UIPushQuad(PushBuffer *push_buffer, const u32 x, const u32 y, const u32 w, const u32 h, const Vec4 color) {
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
void UIPushText(PushBuffer *push_buffer, const char *text, const u32 x, const u32 y, const Vec4 color) {
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

void UIPushFmt(PushBuffer *push_buffer, const u32 x, const u32 y, const Vec4 color, const char *fmt, ...) {
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

void UIPushTexture(PushBuffer *push_buffer, const u32 texture, const u32 x, const u32 y, const u32 w, const u32 h) {
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
void PushMesh(PushBuffer *push_buffer, const MeshHandle mesh, TransformHandle transform, Vec3 diffuse_color) {
    ASSERT(push_buffer->size + sizeof(PushBufferEntryMesh) < push_buffer->max_size);
    PushBufferEntryMesh *entry = (PushBufferEntryMesh *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Mesh;
    entry->mesh = mesh;
    entry->transform = transform;
    entry->diffuse_color = diffuse_color;
    
    push_buffer->size += sizeof(PushBufferEntryMesh);
}

void PushSkin(PushBuffer *push_buffer, MeshHandle mesh, SkinHandle skin, TransformHandle xform, Vec3 diffuse_color) {
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
void DebugPushMatrix(PushBuffer *push_buffer, Mat4 matrix) {
    
    ASSERT(push_buffer->size + sizeof(PushBufferEntryAxisGizmo) < push_buffer->max_size);
    PushBufferEntryAxisGizmo *entry = (PushBufferEntryAxisGizmo *)(push_buffer->buf + push_buffer->size);
    
    entry->type = PushBufferEntryType_AxisGizmo;
    entry->line[0] = mat4_mul_vec3(matrix, (Vec3){0.0f, 0.0f, 0.0f});
    entry->line[1] = mat4_mul_vec3(matrix, (Vec3){0.1f, 0.0f, 0.0f});
    entry->line[2] = mat4_mul_vec3(matrix, (Vec3){0.0f, 0.1f, 0.0f});
    entry->line[3] = mat4_mul_vec3(matrix, (Vec3){0.0f, 0.0f, 0.1f});
    
    push_buffer->size += sizeof(PushBufferEntryAxisGizmo);
}

void DebugPushPosition(PushBuffer *push_buffer, Vec3 position) {
    ASSERT(push_buffer->size + sizeof(PushBufferEntryAxisGizmo) < push_buffer->max_size);
    PushBufferEntryAxisGizmo *entry = (PushBufferEntryAxisGizmo *)(push_buffer->buf + push_buffer->size);
    
    entry->type = PushBufferEntryType_AxisGizmo;
    entry->line[0] = position;
    entry->line[1] = vec3_add(position, VEC3_RIGHT);
    entry->line[2] = vec3_add(position, VEC3_UP);
    entry->line[3] = vec3_add(position, VEC3_FORWARD);
    
    push_buffer->size += sizeof(PushBufferEntryAxisGizmo);
}
