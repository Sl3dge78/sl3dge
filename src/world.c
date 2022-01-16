
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
    e->color = (Vec3){1.0f, 1.0f, 1.0f};
    return world->entity_count++;
}

inline Entity *WorldGetEntity(World *world, EntityID id) {
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

void UpdateAndDrawEntities(GameData *game_data, World *world, Input *input, Renderer *renderer, f32 delta_time) {

    for(u32 id = 0; id < world->entity_count; id++) {
        Entity *e = WorldGetEntity(world, id);
        if(!(e->flags & EntityFlag_Sleeping)) {
            // Update entity
            switch(e->type) {
                case (EntityType_Player): {
                    UpdatePlayer(game_data, input, e, delta_time);
                } break;
                case (EntityType_Sword): {
                    UpdateSword(game_data, e);
                } break;
                case (EntityType_Enemy): {
                    UpdateEnemy(game_data, e);    
                } break;
                case (EntityType_NPC): {
                    UpdateNPC(world, &game_data->npc, delta_time);
                }
                default : break;
            }
        }
        if(!(e->flags & EntityFlag_Hidden)) {
            // Draw entity
            switch(e->render_type) {
                case(RenderingType_StaticMesh) : {
                    PushMesh(&renderer->scene_pushbuffer, e->static_mesh, &e->transform, e->color);
                } break;
                case(RenderingType_SkinnedMesh) : {
                    PushSkinnedMesh(&renderer->scene_pushbuffer, e->skinned_mesh, &e->transform, e->skeleton, e->color);
                } break;
            }
        }
    }
}
