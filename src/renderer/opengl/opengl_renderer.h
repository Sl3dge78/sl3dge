#pragma once

#include "utils/sl3dge.h"
#include <stb_truetype.h>

typedef struct ShadowmapRenderPass {
    u32 framebuffer;
    u32 texture;
    
    u32 pipeline;
    u32 size;
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

typedef struct RendererBackend {
    ShadowmapRenderPass shadowmap_pass;
    ColorRenderPass color_pass;
    VolumetricRenderPass vol_pass;
    
    u32 screen_quad;
    u32 screen_quad_vbuffer;
    
    // UI
    u32 ui_program;
    u32 ui_vertex_array;
    u32 ui_vertex_buffer;
    
    u32 white_texture;
    
    // Skeleton
    u32 line_program;
    
    // Font
    u32 glyphs_texture;
    stbtt_bakedchar *char_data;
    
    u32 static_mesh_vtx_shader;
    u32 skinned_mesh_vtx_shader;
    
    u32 color_fragment_shader;
} RendererBackend;

typedef RendererBackend OpenGLRenderer;

void GLLoadFunctions();