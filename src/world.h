#pragma once

typedef struct World {
    Entity *entities;
    u32 entity_count;
    u32 size;
} World;


void WorldInit(World *world, u32 size);
EntityID WorldCreateEntity(World *world);
inline Entity *WorldGetEntity(World *world, EntityID id);
EntityID WorldCreateAndGetEntity(World *world, Entity **result);
void WorldDestroy(World *world);
void UpdateAndDrawEntities(GameData *game_data, World *world, Input *input, Renderer *renderer, f32 delta_time);
