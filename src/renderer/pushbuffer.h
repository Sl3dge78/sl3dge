#pragma once

typedef u32 MeshHandle;
typedef u32 SkinnedMeshHandle;

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
    PushBufferEntryType_SkinnedMesh,
    PushBufferEntryType_Texture,
    PushBufferEntryType_AxisGizmo,
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
    MeshHandle mesh;
    Transform* transform;
    Vec3 diffuse_color;
} PushBufferEntryMesh;

typedef struct PushBufferEntrySkinnedMesh {
    PushBufferEntryType type;
    SkinnedMeshHandle skin;
    Transform* transform;
    Transform* skeleton;
    Vec3 diffuse_color;
} PushBufferEntrySkinnedMesh;

typedef struct PushBufferEntryAxisGizmo {
    PushBufferEntryType type;
    Vec3 line[4]; // 0 = bone origin, 1 = X axis, 2 = Y axis, 3 = Z axis
} PushBufferEntryAxisGizmo;

typedef struct PushBufferEntryTexture {
    PushBufferEntryType type;
    u32 l, t, r, b;
    u32 texture;
} PushBufferEntryTexture;

void PushMesh(PushBuffer *push_buffer, const MeshHandle mesh, Transform *transform, Vec3 diffuse_color);
void PushSkinnedMesh(PushBuffer *push_buffer, const SkinnedMeshHandle skinned_mesh, Transform *root, Transform *skeleton_root, Vec3 diffuse_color);
void UIPushQuad(PushBuffer *push_buffer, const u32 x, const u32 y, const u32 w, const u32 h, const Vec4 color);
void UIPushText(PushBuffer *push_buffer, const char *text, const u32 x, const u32 y, const Vec4 color);
void UIPushFmt(PushBuffer *push_buffer, const u32 x, const u32 y, const Vec4 color, const char *fmt, ...);
void UIPushTexture(PushBuffer *push_buffer, const u32 texture, const u32 x, const u32 y, const u32 w, const u32 h);
void DebugPushMatrix(PushBuffer *push_buffer, Mat4 matrix);
void DebugPushPosition(PushBuffer *push_buffer, Vec3 position);
