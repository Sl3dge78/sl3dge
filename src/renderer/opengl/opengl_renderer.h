#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include <sl3dge-utils/sl3dge.h>
#include <stb/stb_truetype.h>

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

typedef struct SkinnedMesh {
    MeshHandle mesh;
    u32 joint_count;
    Mat4 *joints;
    i32 *joint_parents;
    u32 *joint_children_count;
    u32 **joint_children;
    Mat4 *inverse_bind_matrices;
} SkinnedMesh;

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

    PushBuffer scene_pushbuffer;
    //Mesh moto;
    Mesh *meshes;
    u32 mesh_capacity;
    u32 mesh_count;

    // Uniform data
    Mat4 camera_proj;
    Mat4 camera_proj_inverse;
    Mat4 camera_view;
    Mat4 camera_view_inverse;
    Vec3 camera_pos;
    Mat4 light_matrix;
    Vec3 light_dir;

    // UI
    u32 ui_program;
    u32 ui_vertex_array;
    u32 ui_vertex_buffer;
    PushBuffer ui_pushbuffer;
    u32 white_texture;

    // Font
    u32 glyphs_texture;
    stbtt_bakedchar *char_data;

} Renderer;

void GLLoadFunctions();

#endif