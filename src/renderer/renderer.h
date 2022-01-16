#pragma once

// Forward declarations
typedef struct GameData GameData;
struct RendererBackend;

typedef struct Vertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
} Vertex;

typedef struct SkinnedVertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
    u8 joints[4];
    f32 weights[4];
} SkinnedVertex;

typedef struct Mesh {
    u32 vertex_array;
    u32 vertex_buffer;
    u32 index_buffer;
    
    u32 vertex_count;
    u32 index_count;
} Mesh;

typedef u32 MeshHandle;

typedef struct SkinnedMesh {
    Mesh mesh;

    u32 joint_count;
    
    // Le skin stocke les transforms de chaque joint.
    // Il faut ensuite les dupliquer dans une nouvelle hiérarchie pour obtenir une instance de squelette complet
    // C'est sur ce nouveau skelette qu'il faudra influer et non sur celui-ci.
    // Il est read only.
    Transform *joint_xforms;
    Mat4 *global_joint_mats;
    
    i32 *joint_parents;
    
    u32 *joint_child_count;
    u32 **joint_children;
    Mat4 *inverse_bind_matrices;
} SkinnedMesh;

typedef u32 SkinnedMeshHandle;

// --------
// Animation

typedef enum AnimationType {
    ANIM_TYPE_QUATERNION,
    ANIM_TYPE_VEC3,
    ANIM_TYPE_FLOAT,
} AnimationType;

typedef enum AnimationTarget {
    ANIM_TARGET_TRANSLATION,
    ANIM_TARGET_ROTATION,
    ANIM_TARGET_SCALE,
} AnimationTarget;

typedef struct AnimationTrack {
    AnimationType type;
    u32 key_count;
    f32 *key_times;
    void *keys;
    
    u32 target_node;
    AnimationTarget target;
    
} AnimationTrack;

typedef struct Animation {
    f32 length;
    u32 track_count;
    AnimationTrack *tracks;
} Animation;

typedef u32 AnimationHandle;

// --------
// Renderer
typedef struct Renderer {
    struct RendererBackend *backend;
    PlatformWindow *window;
    
    u32 width;
    u32 height;
    
    PushBuffer scene_pushbuffer;
    PushBuffer ui_pushbuffer;
    PushBuffer debug_pushbuffer;
    
    sArray meshes;
    sArray skins;
    sArray animations;
    //sArray transforms;
    
    // Uniform data
    Mat4 camera_proj;
    Mat4 camera_proj_inverse;
    Mat4 camera_view;
    Mat4 camera_view_inverse;
    Mat4 camera_vp;
    Vec3 camera_pos;
    Mat4 light_matrix;
    Vec3 light_dir;
} Renderer;

void SkinCalcChildXform(u32 joint_id, SkinnedMesh *skin, Transform *skeleton);
void UpdateCameraProj(Renderer *renderer);

MeshHandle LoadMeshFromVertices(Renderer *renderer, const Vertex *vertices, const u32 vertex_count, const u32 *indices, const u32 index_count);
void LoadFromGLTF(const char *path, Renderer *renderer, PlatformAPI *platform, MeshHandle *mesh, SkinnedMeshHandle *skin, Animation *animation);
MeshHandle LoadQuad(Renderer *renderer);
MeshHandle LoadCube(Renderer *renderer);

void LoadAnimation(Animation *animation, const GLTF *gltf);
void DestroyAnimation(Animation *anim);
void AnimationEvaluate(const Animation *animation, Transform *target, f32 time);

void RendererSetCamera(Renderer *renderer, const Mat4 view, const Vec3 pos);
void RendererSetSunDirection(Renderer *renderer, const Vec3 direction);


