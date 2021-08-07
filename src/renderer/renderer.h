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

typedef u32 MeshHandle;

// -----------
// Push Buffer

typedef struct PushBuffer {
    u32 size;
    u32 max_size;
    void *buf;
} PushBuffer;

typedef enum PushBufferEntryType {
    PushBufferEntryType_Quad,
    PushBufferEntryType_Text,
    PushBufferEntryType_Mesh
} PushBufferEntryType;

typedef struct PushBufferEntryQuad {
    PushBufferEntryType type;
    u32 l, t, r, b;
    Vec4 colour;
} PushBufferEntryQuad;

typedef struct PushBufferEntryText {
    PushBufferEntryType type;
    const char *text;
    u32 x, y;
    Vec4 colour;
} PushBufferEntryText;

typedef struct PushBufferEntryMesh {
    PushBufferEntryType type;
    MeshHandle mesh_handle;
    Mat4 *transform;
} PushBufferEntryMesh;

// Platform level functions
Renderer *
RendererCreate(PlatformWindow *window);
void RendererDrawFrame(Renderer *renderer);
void RendererDestroy(Renderer *renderer);
void RendererUpdateWindow(Renderer *renderer, PlatformWindow *window);
PushBuffer *RendererGetUIPushBuffer(Renderer *renderer);
PushBuffer *RendererGetScenePushBuffer(Renderer *renderer);

// Game functions
typedef u32 CreateMesh_t(Renderer *renderer, const char *path);
DLL_EXPORT CreateMesh_t RendererCreateMesh;

//typedef MeshInstance InstantiateMesh_t(Renderer *renderer, u32 mesh_id);
//DLL_EXPORT InstantiateMesh_t RendererInstantiateMesh;

typedef void SetCamera_t(Renderer *renderer, const Mat4 *view);
DLL_EXPORT SetCamera_t RendererSetCamera;

typedef void SetSunDirection_t(Renderer *renderer, const Vec3 direction);
DLL_EXPORT SetSunDirection_t RendererSetSunDirection;

typedef struct RendererGameAPI {
    CreateMesh_t *CreateMesh;
    //InstantiateMesh_t *InstantiateMesh;
    SetCamera_t *SetCamera;
    SetSunDirection_t *SetSunDirection;
} RendererGameAPI;

#endif
