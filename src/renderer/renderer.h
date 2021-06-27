#ifndef RENDERER_H
#define RENDERER_H

#include <sl3dge/sl3dge.h>

#include "platform/platform.h"

// Forward declarations

struct Renderer;
struct GameData;
struct Frame;
struct Buffer;
struct Mesh;

// Platform level functions

typedef Renderer *CreateRenderer_t(SDL_Window *window, PlatformAPI *platform_api);
CreateRenderer_t *pfn_CreateRenderer;

typedef void DestroyRenderer_t(Renderer *renderer);
DestroyRenderer_t *pfn_DestroyRenderer;

typedef void RendererReloadShaders_t(Renderer *renderer);
RendererReloadShaders_t *pfn_ReloadShaders;

typedef void DrawFrame_t(Renderer *renderer, GameData *game_data);
DrawFrame_t *pfn_DrawFrame;

struct RendererPlatformAPI {
    CreateRenderer_t *CreateRenderer;
    DestroyRenderer_t *DestroyRenderer;
    RendererReloadShaders_t *ReloadShaders;
    DrawFrame_t *DrawFrame;
};

// Game functions
typedef u32 LoadMesh_t(Renderer *renderer, const char *path);
DLL_EXPORT LoadMesh_t RendererLoadMesh;

typedef void DestroyMesh_t(Renderer *renderer, u32 mesh);
DLL_EXPORT DestroyMesh_t RendererDestroyMesh;

typedef void DrawMesh_t(Frame *frame, Mesh *mesh);
DrawMesh_t RendererDrawMesh;

typedef void InstantiateMesh_t(Renderer *renderer, u32 mesh);
DLL_EXPORT InstantiateMesh_t RendererInstantiateMesh;

struct RendererGameAPI {
    LoadMesh_t *LoadMesh;
    DestroyMesh_t *DestroyMesh;
    InstantiateMesh_t *InstantiateMesh;
};

// Declarations

typedef struct Primitive {
    u32 material_id;
    u32 node_id;
    u32 index_count;
    u32 index_offset;
    u32 vertex_count;
    u32 vertex_offset;
} Primitive;

struct Mesh {
    Buffer *buffer;       // idx & vtx buffer
    u32 all_index_offset; // Indices start at this offset in the buffer

    u32 total_vertex_count;
    u32 total_index_count;
    u32 total_primitives_count;
    Primitive *primitives;

    u32 primitive_nodes_count;
    Mat4 *primitive_transforms;

    u32 instance_count;
    Mat4 *instance_transforms;
};

typedef struct PushConstant {
    alignas(16) Mat4 transform;
    alignas(4) u32 material;
} PushConstant;

typedef struct Vertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
} Vertex;

typedef struct Material {
    alignas(16) Vec3 base_color;
    alignas(4) u32 base_color_texture;
    alignas(4) u32 metallic_roughness_texture;
    alignas(4) float metallic_factor;
    alignas(4) float roughness_factor;
    alignas(4) u32 normal_texture;
    alignas(4) u32 ao_texture;
    alignas(4) u32 emissive_texture;
} Material;

#endif