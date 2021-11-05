#pragma once

typedef struct Mesh Mesh;
typedef struct Skin Skin;

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
    PushBufferEntryType_Skin,
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
    Mesh *mesh;
    Transform *transform;
    Vec3 diffuse_color;
} PushBufferEntryMesh;

typedef struct PushBufferEntrySkin {
    PushBufferEntryType type;
    Mesh *mesh;
    Skin *skin;
    Transform *transform;
    Vec3 diffuse_color;
} PushBufferEntrySkin;

typedef struct PushBufferEntryBone {
    PushBufferEntryType type;
    Vec3 line[4]; // 0 = bone origin, 1 = X axis, 2 = Y axis, 3 = Z axis
} PushBufferEntryBone;

typedef struct PushBufferEntryTexture {
    PushBufferEntryType type;
    u32 l, t, r, b;
    u32 texture;
} PushBufferEntryTexture;
