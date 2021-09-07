#pragma once

#include "utils/sl3dge.h"

#include "platform/platform.h"
#include "renderer/renderer_api.h"

// Forward declarations
typedef struct Renderer Renderer;
typedef struct GameData GameData;

typedef struct Vertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
} Vertex;

typedef struct SkinnedVertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
    u16 joints[4];
    f32 weights[4];
} SkinnedVertex;

typedef u32 MeshHandle;

typedef struct SkinnedMesh {
    MeshHandle mesh;
    u32 joint_count;
    Mat4 *joints;
    i32 *joint_parents;
    u32 *joint_children_count;
    u32 **joint_children;
    Mat4 *inverse_bind_matrices;
} SkinnedMesh;

// -----------
// Push Buffer

typedef struct PushBuffer {
    u32 size;
    u32 max_size;
    void *buf;
} PushBuffer;

typedef enum PushBufferEntryType {
    PushBufferEntryType_UIQuad,
    PushBufferEntryType_Text,
    PushBufferEntryType_Mesh,
    PushBufferEntryType_Texture,
    PushBufferEntryType_Bone,
} PushBufferEntryType;

typedef struct PushBufferEntryUIQuad {
    PushBufferEntryType type;
    u32 l, t, r, b;
    Vec4 colour;
} PushBufferEntryUIQuad;

typedef struct PushBufferEntryText {
    PushBufferEntryType type;
    char *text;
    u32 x, y;
    Vec4 colour;
} PushBufferEntryText;

typedef struct PushBufferEntryMesh {
    PushBufferEntryType type;
    MeshHandle mesh_handle;
    Mat4 *transform;
    Vec3 diffuse_color;
} PushBufferEntryMesh;

typedef struct PushBufferEntryBone {
    PushBufferEntryType type;
    Vec3 line[2];
} PushBufferEntryBone;

typedef struct PushBufferEntryTexture {
    PushBufferEntryType type;
    u32 l, t, r, b;
    u32 texture;
} PushBufferEntryTexture;

// Game functions
MeshHandle RendererLoadMesh(Renderer *renderer, const char *path);
MeshHandle RendererLoadMeshFromVertices(Renderer *renderer, const Vertex *vertices, const u32 vertex_count, const u32 *indices, const u32 index_count);
void RendererSetCamera(Renderer *renderer, const Mat4 *view, const Vec3 pos);
void RendererSetSunDirection(Renderer *renderer, const Vec3 direction);

internal void UIPushQuad(Renderer *renderer, const u32 x, const u32 y, const u32 w, const u32 h, const Vec4 color);
internal void UIPushText(Renderer *renderer, const char *text, const u32 x, const u32 y, const Vec4 color);
internal void PushMesh(Renderer *renderer, MeshHandle mesh, Mat4 *transform, Vec3 diffuse_color);

