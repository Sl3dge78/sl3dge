#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include <sl3dge-utils/sl3dge.h>

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

    u32 vertex_shader;
    u32 fragment_shader;
    u32 shader_program;

    u32 mesh_count;
    Mesh **meshes;
    u32 materials_count;

    Mat4 camera_proj;
    Mat4 camera_view;
    Mat4 camera_view_inverse;
    Vec3 camera_pos;
} Renderer;

typedef struct Frame {
    // TODO
} Frame;

void GLLoadFunctions();

#endif