#include "renderer/renderer.h"
#include "renderer/opengl/opengl_renderer.h"

#include <gl/GL.h>

Renderer *RendererCreate(PlatformWindow *window, PlatformAPI *platform_api) {
    sLog("Init Opengl. Version %d", glGetString(GL_VERSION));
    return 0;
}

void RendererDrawFrame(Renderer *renderer) {
}

void RendererDestroy(Renderer *renderer) {
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
