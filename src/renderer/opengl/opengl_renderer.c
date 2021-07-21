#include "renderer/renderer.h"
#include "renderer/opengl/opengl_renderer.h"

#include <gl/GL.h>

#ifdef __WIN32__
#include "renderer/opengl/opengl_win32.c"
#endif

Renderer *RendererCreate(PlatformWindow *window, PlatformAPI *platform_api) {
    Renderer *renderer = sMalloc(sizeof(Renderer));
    renderer->window = window;
    PlatformCreateOpenGLContext(renderer, window);

    sLog("Init Opengl. Version %d", glGetString(GL_VERSION));
    return renderer;
}

void RendererDrawFrame(Renderer *renderer) {
    glViewport(0, 0, renderer->width, renderer->height);
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    PlatformSwapBuffers(renderer);
}

void RendererDestroy(Renderer *renderer) {
    sFree(renderer);
}

void RendererUpdateWindow(Renderer *renderer, PlatformWindow *window) {
}

u32 RendererLoadMesh(Renderer *renderer, const char *path) {
    return 0;
}

void RendererDestroyMesh(Renderer *renderer, u32 mesh) {
}

MeshInstance RendererInstantiateMesh(Renderer *renderer, u32 mesh_id) {
    MeshInstance result = {0};
    return result;
}

void RendererSetCamera(Renderer *renderer, const Vec3 position, const Vec3 forward, const Vec3 up) {
}

void RendererSetSunDirection(Renderer *renderer, const Vec3 direction) {
}
