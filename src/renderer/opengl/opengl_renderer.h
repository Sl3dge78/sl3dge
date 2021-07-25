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
    u32 index_count;
    u32 diffuse_texture;
} Mesh;

typedef struct Renderer {
    PlatformWindow *window;
    u32 width;
    u32 height;

    u32 vertex_shader;
    u32 fragment_shader;
    u32 shader_program;

    Mesh moto;

    u32 mesh_count;
    Mesh **meshes;

    Mat4 camera_proj;
    Mat4 camera_view;
    Mat4 camera_view_inverse;
    Vec3 camera_pos;

    u32 texture_count;
    u32 textures[12];

} Renderer;

typedef struct Frame {
    // TODO
} Frame;

void GLLoadFunctions();

#endif