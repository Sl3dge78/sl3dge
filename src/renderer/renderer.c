#include "renderer/renderer.h"

#ifdef RENDERER_VULKAN
#include "renderer/vulkan/vulkan_renderer.c"
#elif RENDERER_OPENGL
#include "renderer/opengl/opengl_renderer.c"
#endif

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
internal void PushMesh(Renderer *renderer, MeshHandle mesh, Mat4 *transform, Vec3 diffuse_color) {
    PushBuffer *push_buffer = &renderer->scene_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntryMesh) < push_buffer->max_size);
    PushBufferEntryMesh *entry = (PushBufferEntryMesh *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Mesh;
    entry->mesh_handle = mesh;
    entry->transform = transform;
    entry->diffuse_color = diffuse_color;
    
    push_buffer->size += sizeof(PushBufferEntryMesh);
}

/// Adds a bone to the scene draw calls
/// If child is null, draw a bone of length of 1
internal void PushBone(Renderer *renderer, Mat4 *parent, Mat4 *child) {
    PushBuffer *push_buffer = &renderer->debug_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntryBone) < push_buffer->max_size);
    PushBufferEntryBone *entry = (PushBufferEntryBone *)(push_buffer->buf + push_buffer->size);
    
    entry->type = PushBufferEntryType_Bone;
    entry->line[0] = mat4_mul_vec3(parent, (Vec3){0.0f, 0.0f, 0.0f});
    if(child)
        entry->line[1] = mat4_mul_vec3(child, (Vec3){0.0f, 0.0f, 0.0f});
    else {
        entry->line[1] = mat4_mul_vec3(parent, (Vec3){0.0f, 1.0f, 0.0f});
    }
    push_buffer->size += sizeof(PushBufferEntryBone);
    
}