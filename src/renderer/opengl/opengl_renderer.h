#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include "utils/sl3dge.h"
#include <stb_truetype.h>

#include "platform/platform.h"
#include "renderer/renderer.h"

typedef struct ShadowmapRenderPass {
    u32 framebuffer;
    u32 texture;
    
    u32 pipeline;
    
} ShadowmapRenderPass;

typedef struct ColorRenderPass {
    u32 framebuffer;
    u32 render_target;
    
    u32 pipeline;
    
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
    
    Mesh *meshes;
    u32 mesh_capacity;
    u32 mesh_count;
    
    SkinnedMesh *skinned_meshes;
    u32 skinned_mesh_capacity;
    u32 skinned_mesh_count;
    
    Transform *transforms;
    u32 transform_capacity;
    u32 transform_count;
    
    // Uniform data
    Mat4 camera_proj;
    Mat4 camera_proj_inverse;
    Mat4 camera_view;
    Mat4 camera_view_inverse;
    Mat4 camera_vp;
    Vec3 camera_pos;
    Mat4 light_matrix;
    Vec3 light_dir;
    
    // UI
    u32 ui_program;
    u32 ui_vertex_array;
    u32 ui_vertex_buffer;
    PushBuffer ui_pushbuffer;
    u32 white_texture;
    
    // Skeleton
    u32 line_program;
    PushBuffer debug_pushbuffer;
    
    // Font
    u32 glyphs_texture;
    stbtt_bakedchar *char_data;
    
    u32 static_mesh_vtx_shader;
    u32 skinned_mesh_vtx_shader;
    
    u32 color_fragment_shader;
    
    
} Renderer;

void GLLoadFunctions();

#endif