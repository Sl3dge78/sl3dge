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

    u32 shadowmap_framebuffer;
    u32 shadowmap_texture;
    u32 shadowmap_program;

    u32 main_framebuffer;
    u32 main_renderbuffer;
    u32 main_render_target;
    u32 main_program;
    u32 main_depthmap;

    u32 screen_quad;
    u32 screen_quad_vbuffer;
    u32 postprocess_program;

    Mesh moto;

    u32 mesh_count;
    Mesh **meshes;

    Mat4 camera_proj;
    Mat4 camera_proj_inverse;
    Mat4 camera_view;
    Mat4 camera_view_inverse;
    Vec3 camera_pos;

    Mat4 light_matrix;
    Vec3 light_dir;

} Renderer;

typedef struct Frame {
    // TODO
} Frame;

void GLLoadFunctions();

#endif