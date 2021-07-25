#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include "platform/platform.h"
#include "renderer/renderer.h"

typedef struct Mesh {
    u32 vertex_array;
    u32 vertex_buffer;
    u32 index_buffer;

    u32 vertex_count;
} Mesh;

typedef struct Renderer {
    PlatformWindow *window;
    u32 width;
    u32 height;
    u32 vbo;
    u32 vertex_shader;
    u32 fragment_shader;
    u32 shader_program;
    u32 vao;

    u32 mesh_count;
    Mesh **meshes;
    u32 materials_count;
} Renderer;

typedef struct Frame {
    // TODO
} Frame;

void GLLoadFunctions();

#endif