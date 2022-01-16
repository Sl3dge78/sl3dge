
EntityID CreatePlayer(World *world, MeshHandle mesh) {
    Entity *player_entity;
    EntityID result = WorldCreateAndGetEntity(world, &player_entity);
    player_entity->static_mesh = mesh;
    player_entity->type = EntityType_Player;
    return result;
}

EntityID CreateSword(World *world) {
    Entity *sword_entity;
    EntityID result = WorldCreateAndGetEntity(world, &sword_entity);
    sword_entity->color = (Vec3){0.0f, 0.0f, 0.0f};
    sword_entity->static_mesh = MakeCuboid(global_renderer, (Vec3){-.1f, -.1f, .2f}, (Vec3){.1f, .1f, 1.f});
    sword_entity->flags = EntityFlag_Hidden | EntityFlag_Sleeping;
    sword_entity->type = EntityType_Sword;
    return result;
}

EntityID CreateEnemy(World *world, MeshHandle mesh) {
    static u32 i = 0;

    Entity *enemy_entity;
    EntityID result = WorldCreateAndGetEntity(world, &enemy_entity);
    enemy_entity->transform.translation.x = i * 2.0f;
    i++;
    enemy_entity->static_mesh = mesh;
    enemy_entity->color = (Vec3){1.0f, 0.0f, 0.0f};
    enemy_entity->type = EntityType_Enemy;
    enemy_entity->health = 10;
    return result;
}

void CreateNPC(World *world, Renderer *renderer, PlatformAPI *platform, NPC *npc) {
    SkinnedMeshHandle sk;
    LoadFromGLTF("resources/3d/character/walk.gltf", renderer, platform, NULL, &sk, &npc->walk_animation);
    npc->entity = InstantiateSkin(global_renderer, world, sk);
    Entity *npc_e = WorldGetEntity(world, npc->entity);
    npc_e->type = EntityType_NPC;
    npc->anim_time = 0.0f;
    npc->walk_speed = 1.0f;
}

void UpdatePlayer(GameData *game_data, Input *input, Entity *e, f32 delta_time) {

    Entity *sword = WorldGetEntity(&game_data->world, game_data->sword);
    if(input) {
        // Move the character
        f32 move_speed = 10.0f;
        Vec3 movement = {0};
        if(input->keyboard[SCANCODE_W]) {
            movement.z -= 1.0f;
            if(game_data->attack_time <= 0.0f)
                e->direction = DIRECTION_UP;
        }
        if(input->keyboard[SCANCODE_S]) {
            movement.z += 1.0f;
            if(game_data->attack_time <= 0.0f)
                e->direction = DIRECTION_DOWN;
        }
        if(input->keyboard[SCANCODE_A]) {
            movement.x -= 1.0f;
            if(game_data->attack_time <= 0.0f)
                e->direction = DIRECTION_LEFT;
        }
        if(input->keyboard[SCANCODE_D]) {
            movement.x += 1.0f;
            if(game_data->attack_time <= 0.0f)
                e->direction = DIRECTION_RIGHT;
        }

        movement = vec3_fmul(movement, delta_time * move_speed);
        e->transform.translation = vec3_add(e->transform.translation, movement);

        // Attack
        if (InputKeyPressed(input, SCANCODE_SPACE)) {
            // Do attack
            sword->flags = 0; // Display & update sword
            game_data->attack_time = 0.1f;
            sword->direction = e->direction;
        }
    }
     
    if(game_data->attack_time > 0.0f) {
        game_data->attack_time -= delta_time;
        if(game_data->attack_time <= 0.0f) {
            sword->flags = EntityFlag_Hidden | EntityFlag_Sleeping;
        }
    }
}

void UpdateSword(GameData *game_data, Entity *e) {
    Quat start_rot;
    Quat end_rot;
    Vec3 offset;
    switch(e->direction) {
        case DIRECTION_UP: {
            start_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, PI); 
            end_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, -HALF_PI); 
            offset = (Vec3) {0.5f, 0.5f, 0.0f};
        } break;
        case DIRECTION_DOWN: { 
            start_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, 0.0f); 
            end_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, HALF_PI); 
            offset = (Vec3) {-0.5f, 0.5f, 0.5f};
        } break;
        case DIRECTION_LEFT: {
            start_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, -HALF_PI); 
            end_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, 0.0f); 
            offset = (Vec3) {-0.5f, 0.5f, -0.5f};
        } break;
        case DIRECTION_RIGHT: {
            start_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, HALF_PI); 
            end_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, PI); 
            offset = (Vec3) {0.5f, 0.5f, 0.5f};
        } break;
    }

    Entity *player = WorldGetEntity(&game_data->world, game_data->player);
    e->transform.rotation = quat_slerp(end_rot, start_rot, game_data->attack_time / 0.1f ); 
    e->transform.translation = vec3_add(player->transform.translation, offset);
}

Vec3 SwordGetTip(Entity *sword) {
    Vec3 forward = vec3_fmul(VEC3_FORWARD, 1.0f);
    forward = vec3_rotate(forward, sword->transform.rotation); 
    return vec3_add(forward, sword->transform.translation); 
}

void UpdateEnemy(GameData *game_data, Entity *e) {
    Entity *sword = WorldGetEntity(&game_data->world, game_data->sword);
    bool is_sleeping = sword->flags & EntityFlag_Sleeping;
    if(!is_sleeping && IsLineIntersectingBoundingBox(sword->transform.translation, SwordGetTip(sword), &e->transform)) { 
        if(!e->collided) { // On enter...
            e->health--;
            if(e->health <= 0) {
                e->flags |= EntityFlag_Hidden;
            }
            e->color = (Vec3){1.0f, 1.0f, 1.0f};
        }
        e->collided = true;
    } else if(e->collided) { // On exit...
        e->collided = false;
        e->color = (Vec3){1.0f, 0.0f, 0.0f};
    }
    
}

void UpdateNPC(World *world, NPC *npc, f32 delta_time) {
    Entity *e = WorldGetEntity(world, npc->entity);

    npc->anim_time = fmod(npc->anim_time + delta_time, npc->walk_animation.length);
    AnimationEvaluate(&npc->walk_animation, e->skeleton, npc->anim_time);

    Vec3 diff = vec3_sub(npc->destination, e->transform.translation);
    npc->distance_to_dest = vec3_length(diff);

    npc->walk_speed = 1.3f;

    if(npc->distance_to_dest > 0.1f) {
        Vec3 dir = vec3_normalize(diff);
        e->transform.translation = vec3_add(vec3_fmul(dir, npc->walk_speed * delta_time), e->transform.translation);
    } else {
        // Get new dest
        static u32 i = 0;
        if(i == 0) {
            npc->destination = (Vec3) {0.0f, 0.0f, 0.0f};
            e->transform.rotation = quat_lookat(e->transform.translation, npc->destination, (Vec3){0.0f, 1.0f, 0.0f});
            i++;
        } else if(i == 1) {
            npc->destination = (Vec3) {5.0f, 0.0f, 0.0f};
            e->transform.rotation = quat_lookat(e->transform.translation, npc->destination, (Vec3){0.0f, 1.0f, 0.0f});
            i++;
        } else if(i == 2) {
            npc->destination = (Vec3) {5.0f, 0.0f, -5.0f};
            e->transform.rotation = quat_lookat(e->transform.translation, npc->destination, (Vec3){0.0f, 1.0f, 0.0f});
            i++;
        } else if (i == 3) {
            npc->destination = (Vec3) {0.0f, 0.0f, -5.0f};
            e->transform.rotation = quat_lookat(e->transform.translation, npc->destination, (Vec3){0.0f, 1.0f, 0.0f});
            i = 0;
        }
    }
}
