#include "renderer/renderer.h"

#ifdef RENDERER_VULKAN
#include "renderer/vulkan/vulkan_renderer.c"
#elif RENDERER_OPENGL
#include "renderer/opengl/opengl_renderer.c"
#endif

internal void UIPushQuad(Renderer *renderer, const u32 x, const u32 y, const u32 w, const u32 h, const Vec4 color) {
    PushBuffer *push_buffer = &renderer->ui_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntryQuad) < push_buffer->max_size);
    PushBufferEntryQuad *entry = (PushBufferEntryQuad *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Quad;
    entry->l = x;
    entry->t = y;
    entry->r = w + x;
    entry->b = h + y;
    entry->colour = color;
    push_buffer->size += sizeof(PushBufferEntryQuad);
}

internal void UIPushText(Renderer *renderer, const char *text, const u32 x, const u32 y, const Vec4 color) {
    PushBuffer *push_buffer = &renderer->ui_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntryText) < push_buffer->max_size);
    PushBufferEntryText *entry = (PushBufferEntryText *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Text;
    entry->text = text;
    entry->x = x;
    entry->y = y;
    entry->colour = color;
    push_buffer->size += sizeof(PushBufferEntryText);
}

internal void PushMesh(Renderer *renderer, MeshHandle mesh, Mat4 *transform) {
    PushBuffer *push_buffer = &renderer->scene_pushbuffer;
    ASSERT(push_buffer->size + sizeof(PushBufferEntryText) < push_buffer->max_size);
    PushBufferEntryMesh *entry = (PushBufferEntryMesh *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Mesh;
    entry->mesh_handle = mesh;
    entry->transform = transform;

    push_buffer->size += sizeof(PushBufferEntryMesh);
}