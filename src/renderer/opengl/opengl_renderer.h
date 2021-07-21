#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include "platform/platform.h"

typedef struct Renderer {
    PlatformWindow *window;
    u32 width;
    u32 height;
} Renderer;

typedef struct Frame {
    // TODO
} Frame;

#endif