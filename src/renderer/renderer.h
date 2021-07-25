#ifndef RENDERER_H
#define RENDERER_H

#include <sl3dge-utils/sl3dge.h>

#include "platform/platform.h"

// Forward declarations

typedef struct Renderer Renderer;
typedef struct GameData GameData;

typedef struct Vertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
} Vertex;

typedef struct MeshInstance {
    Mat4 *transform;
} MeshInstance;

// Platform level functions
Renderer *RendererCreate(PlatformWindow *window);
void RendererDrawFrame(Renderer *renderer);
void RendererDestroy(Renderer *renderer);
void RendererUpdateWindow(Renderer *renderer, PlatformWindow *window);

// Game functions
typedef u32 CreateMesh_t(Renderer *renderer, const char *path);
DLL_EXPORT CreateMesh_t RendererCreateMesh;

typedef MeshInstance InstantiateMesh_t(Renderer *renderer, u32 mesh_id);
DLL_EXPORT InstantiateMesh_t RendererInstantiateMesh;

typedef void
SetCamera_t(Renderer *renderer, const Vec3 position, const Vec3 forward, const Vec3 up);
DLL_EXPORT SetCamera_t RendererSetCamera;

typedef void SetSunDirection_t(Renderer *renderer, const Vec3 direction);
DLL_EXPORT SetSunDirection_t RendererSetSunDirection;

typedef struct RendererGameAPI {
    CreateMesh_t *CreateMesh;
    InstantiateMesh_t *InstantiateMesh;
    SetCamera_t *SetCamera;
    SetSunDirection_t *SetSunDirection;
} RendererGameAPI;

#endif