
EntityID CreatePlayer(World *world, MeshHandle mesh) {
    Entity *player_entity;
    EntityID result = WorldCreateAndGetEntity(world, &player_entity);
    player_entity->static_mesh = mesh;
    player_entity->type = EntityType_Player;
    return result;
}

EntityID CreateEnemy(World *world, MeshHandle mesh) {
    Entity *enemy_entity;
    EntityID result = WorldCreateAndGetEntity(world, &enemy_entity);
    enemy_entity->static_mesh = mesh;
    enemy_entity->color = (Vec3){1.0f, 0.0f, 0.0f};
    enemy_entity->type = EntityType_Enemy;
    enemy_entity->health = 10;
    return result;
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
            sword->flags = 0; // Display sword
            game_data->attack_time = 0.1f;
            sword->direction = e->direction;
        }
    }
     
    if(game_data->attack_time > 0.0f) {
        Quat start_rot;
        Quat end_rot;
        Vec3 offset;
        game_data->attack_time -= delta_time;
        if(game_data->attack_time <= 0.0f) {
            sword->flags |= EntityFlag_Hidden;
        }

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

        sword->transform.rotation = quat_slerp(end_rot, start_rot, game_data->attack_time / 0.1f ); 
        sword->transform.translation = vec3_add(e->transform.translation, offset);
        Vec3 forward = vec3_fmul(VEC3_FORWARD, 1.0f);
        forward = vec3_rotate(forward, sword->transform.rotation); 
        Vec3 sword_tip = vec3_add(forward, sword->transform.translation); 
        DebugPushPosition(&global_renderer->debug_pushbuffer, sword_tip); 
        DebugPushPosition(&global_renderer->debug_pushbuffer, sword->transform.translation); 

        Entity *enemy = WorldGetEntity(&game_data->world, game_data->enemy);
        if(enemy->health > 0) {
            if(IsLineIntersectingBoundingBox(sword->transform.translation, sword_tip, &enemy->transform)) { 
                if(enemy->collider_type == 0) {
                    enemy->on_collider_enter = true;
                    enemy->collider_type = sword->type;
                } else {
                    enemy->on_collider_enter = false;
                }
            } else {
                enemy->on_collider_enter = false;
                if(enemy->collider_type != 0) {
                    enemy->on_collider_exit = true;
                } else {
                    enemy->on_collider_exit = false;
                }
            }
        }
    }
}

void UpdateEnemy(Entity *e) {
    if(e->on_collider_enter) {
        e->health--;
        if(e->health <= 0) {
            e->flags |= EntityFlag_Hidden;
        }
        e->color = (Vec3){1.0f, 1.0f, 1.0f};
    }

    if(e->on_collider_exit) {
        e->collider_type = EntityType_None;
        e->color = (Vec3){1.0f, 0.0f, 0.0f};
    }
}
