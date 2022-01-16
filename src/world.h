#pragma once

typedef u32 EntityID;

typedef struct Entity {
    EntityID id;
    Transform transform;
    union {
        struct {
            MeshHandle static_mesh;
        };
        struct {
            SkinnedMeshHandle skinned_mesh;
            Transform *skeleton;
        };
    };
} Entity;

typedef struct World {
    Entity *entities;
    u32 entity_count;
    u32 size;
} World;


void WorldInit(World *world, u32 size) {
    world->entities = sCalloc(size, sizeof(Entity));
    world->size = size;
}

EntityID WorldCreateEntity(World *world) {
    ASSERT(world->entity_count < world->size); // Entity limit
    Entity *e = &world->entities[world->entity_count];
    *e = (Entity){0};
    e->id = world->entity_count;
    transform_identity(&e->transform);
    return world->entity_count++;
}

Entity *WorldGetEntity(World *world, EntityID id) {
    return &world->entities[id];
}

EntityID WorldCreateAndGetEntity(World *world, Entity **result) {
    EntityID e_id = WorldCreateEntity(world);
    *result = WorldGetEntity(world, e_id);
    return e_id;
}

void WorldDestroy(World *world) {
    for(u32 i = 0; i < world->entity_count; i++) {
        Entity *e = WorldGetEntity(world, i);
        // @Clean : this is kind of dirty
        if(e->skeleton) {
            sFree(e->skeleton);
        }
    }
    sFree(world->entities);
}

