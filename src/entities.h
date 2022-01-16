#pragma once

typedef u32 EntityID;

typedef u32 EntityFlags;

typedef enum EntityFlag {
    EntityFlag_Sleeping = 1 << 0, // Don't update
    EntityFlag_Hidden   = 1 << 1, // Don't draw
} EntityFlag;

typedef enum RenderingType {
    RenderingType_StaticMesh,
    RenderingType_SkinnedMesh,
} RenderingType;

typedef enum EntityType {
    EntityType_None = 0,
    EntityType_Player,
    EntityType_Sword,
    EntityType_Enemy,
    EntityType_NPC,
} EntityType;

typedef enum Direction {
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
} Direction;

typedef struct Entity {
    EntityID id;
    EntityFlags flags;
    Transform transform;
    Vec3 color;
    EntityType type;
    RenderingType render_type;
    union {
        struct {
            MeshHandle static_mesh;
        };
        struct {
            SkinnedMeshHandle skinned_mesh;
            Transform *skeleton;
        };
    };

    Direction direction;
    i32 health;
    bool collided;
} Entity;

