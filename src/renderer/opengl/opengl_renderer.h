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

typedef struct ShadowmapRenderPass {
    u32 framebuffer;
    u32 texture;
    u32 program;
} ShadowmapRenderPass;

typedef struct ColorRenderPass {
    u32 framebuffer;
    u32 render_target;
    u32 program;
    u32 depthmap;

    u32 width;
    u32 height;
} ColorRenderPass;

typedef struct VolumetricRenderPass {
    u32 program;
} VolumetricRenderPass;

typedef struct Renderer {
    PlatformWindow *window;
    u32 width;
    u32 height;

    ShadowmapRenderPass shadowmap_pass;
    ColorRenderPass color_pass;
    VolumetricRenderPass vol_pass;

    u32 screen_quad;
    u32 screen_quad_vbuffer;

    Mesh moto;

    // Uniform data
    Mat4 camera_proj;
    Mat4 camera_proj_inverse;
    Mat4 camera_view;
    Mat4 camera_view_inverse;
    Vec3 camera_pos;
    Mat4 light_matrix;
    Vec3 light_dir;

} Renderer;

void GLLoadFunctions();

#endif