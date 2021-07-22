#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include "platform/platform.h"

typedef struct Renderer {
    PlatformWindow *window;
    u32 width;
    u32 height;
    u32 vbo;
    u32 vertex_shader;
    u32 fragment_shader;
    u32 shader_program;
    u32 vao;
} Renderer;

typedef struct Frame {
    // TODO
} Frame;

void GLLoadFunctions();

#endif