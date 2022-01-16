#include <stdint.h>
#include <stdlib.h>

#include "utils/sl3dge.h"
#include "platform/platform.h"

#include "utils/sGltf.c"

#include "renderer/pushbuffer.h"
#include "renderer/renderer.h"

#include "console.h"
#include "event.h"
#include "enemy.h"
#include "world.h"
#include "game.h"

#ifdef RENDERER_VULKAN
#include "renderer/vulkan/vulkan_renderer.c"
//typedef VulkanRenderer Renderer;
#elif RENDERER_OPENGL
#include "renderer/opengl/opengl_renderer.h"
#include "renderer/opengl/opengl_renderer.c"
#include "renderer/opengl/opengl_mesh.c"
#endif

#include "renderer/animation.c"
#include "renderer/pushbuffer.c"
#include "renderer/renderer.c"

global Renderer *global_renderer;
global PlatformAPI *platform;

#include "console.c"
#include "event.c"
#include "collision.c"

// ---------------
// Exported functions

DLL_EXPORT u32 GameGetSize() {
    return sizeof(GameData);
}

/// This is called when the game is (re)loaded
DLL_EXPORT void GameInit(GameData *game_data, Renderer *renderer, PlatformAPI *platform_api) {
    global_renderer = renderer;
    platform = platform_api;
    
    ConsoleInit(&game_data->console);
    global_console = &game_data->console;
    sLogSetCallback(&ConsoleLogMessage);
    
    Leak_SetList(platform_api->DebugInfo);
    WorldInit(&game_data->world, 128);
}

/// This is called ONCE before the first frame. Won't be called upon reloading.
DLL_EXPORT void GameStart(GameData *game_data) {
    
    game_data->light_dir = (Vec3){0.67f, -0.67f, 0.1f};
    game_data->cos = 0;
    RendererSetSunDirection(global_renderer, vec3_normalize(vec3_fmul(game_data->light_dir, -1.0)), (Vec3){0.0f, 0.0f, 0.0f});
    
    platform->SetCaptureMouse(true);
    
    game_data->mesh_quad = LoadQuad(global_renderer);
    game_data->mesh_cube = LoadCube(global_renderer);

    Entity *floor_entity;
    game_data->ground = WorldCreateAndGetEntity(&game_data->world, &floor_entity);
    floor_entity->transform.scale = (Vec3){100.0f, 1.0f, 100.0f};
    floor_entity->static_mesh = game_data->mesh_quad;

    game_data->player = WorldCreateEntity(&game_data->world);

    game_data->enemy = WorldCreateEntity(&game_data->world);
    game_data->enemy_health = 10;

    // Sword
    Entity *sword_entity;
    game_data->sword = WorldCreateAndGetEntity(&game_data->world, &sword_entity);
    sword_entity->static_mesh = MakeCuboid(global_renderer, (Vec3){-.1f, -.1f, .2f}, (Vec3){.1f, .1f, 1.f});

    { // NPC
        NPC *npc = &game_data->npc;
        SkinnedMeshHandle sk;
        LoadFromGLTF("resources/3d/character/walk.gltf", global_renderer, platform, NULL, &sk, &npc->walk_animation);
        npc->entity = InstantiateSkin(global_renderer, &game_data->world, sk);
        npc->anim_time = 0.0f;
        npc->walk_speed = 1.0f;
    }
}

/// Do deallocation here
DLL_EXPORT void GameEnd(GameData *game_data) {
    sFree(game_data->event_queue.queue);
    WorldDestroy(&game_data->world);
    DestroyAnimation(&game_data->npc.walk_animation);
}

internal void FPSCamera(Camera *camera, Input *input, bool is_free_cam) {
    f32 look_speed = 0.01f;
    
    if(input->mouse_delta_x != 0) {
        camera->spherical_coordinates.x += look_speed * input->mouse_delta_x;
    }
    if(input->mouse_delta_y != 0) {
        float new_rot = camera->spherical_coordinates.y + look_speed * input->mouse_delta_y;
        if(new_rot > -PI / 2.0f && new_rot < PI / 2.0f) {
            camera->spherical_coordinates.y = new_rot;
        }
    }
    
    camera->forward = spherical_to_carthesian(camera->spherical_coordinates);
    
    Vec3 flat_forward = vec3_normalize((Vec3){camera->forward.x, 0.0f, camera->forward.z});
    Vec3 right = vec3_cross(flat_forward, (Vec3){0.0f, 1.0f, 0.0f});
    
    // --------------
    // Move
    
    f32 move_speed = 0.1f;
    Vec3 movement = {0};
    
    if(input->keyboard[SCANCODE_LSHIFT]) {
        move_speed *= 5.0f;
    }
    
    if(input->keyboard[SCANCODE_W]) {
        movement = vec3_add(movement, vec3_fmul(flat_forward, move_speed));
    }
    if(input->keyboard[SCANCODE_S]) {
        movement = vec3_add(movement, vec3_fmul(flat_forward, -move_speed));
    }
    if(input->keyboard[SCANCODE_A]) {
        movement = vec3_add(movement, vec3_fmul(right, -move_speed));
    }
    if(input->keyboard[SCANCODE_D]) {
        movement = vec3_add(movement, vec3_fmul(right, move_speed));
    }
    if(is_free_cam) {
        if(input->keyboard[SCANCODE_Q]) {
            movement.y -= move_speed;
        }
        if(input->keyboard[SCANCODE_E]) {
            movement.y += move_speed;
        }
    }
    
    camera->position = vec3_add(camera->position, movement);
    
    if(!is_free_cam) {
        camera->position.y = 1.7f;
    }
    
    Mat4 cam;
    mat4_look_at(vec3_add(camera->forward, camera->position), camera->position, (Vec3){0.0f, 1.0f, 0.0f}, cam);
    
    RendererSetCamera(global_renderer, cam, camera->position);
}

/// Called every frame
DLL_EXPORT void GameLoop(float delta_time, GameData *game_data, Input *input) {
    // --------------
    // Event
    
    {
        EventType e;
        while(EventConsume(&game_data->event_queue, &e)) {
            switch(e) {
                case(EVENT_TYPE_QUIT): {
                    platform->RequestExit();
                } break;
                case(EVENT_TYPE_RESTART): {
                    GameStart(game_data);
                } break;
                case(EVENT_TYPE_RELOAD): {
                    platform->RequestReload();
                } break;
                default: break;
            };
        }
    }
    
    // ----------
    // Input
    
    // Console
    if(InputKeyPressed(input, SCANCODE_TILDE)) {
        ToggleConsole(&game_data->console, input, platform);
    }
    
    if(game_data->console.console_open) {
        InputConsole(&game_data->console, input, game_data);
    } else {
        Entity *player = WorldGetEntity(&game_data->world, game_data->player);
        if(!game_data->is_free_cam) {
            // Move the character
            f32 move_speed = 10.0f;
            Vec3 movement = {0};
            if(input->keyboard[SCANCODE_W]) {
                movement.z -= 1.0f;
                if(game_data->attack_time <= 0.0f)
                    game_data->player_direction = DIRECTION_UP;
            }
            if(input->keyboard[SCANCODE_S]) {
                movement.z += 1.0f;
                if(game_data->attack_time <= 0.0f)
                    game_data->player_direction = DIRECTION_DOWN;
            }
            if(input->keyboard[SCANCODE_A]) {
                movement.x -= 1.0f;
                if(game_data->attack_time <= 0.0f)
                    game_data->player_direction = DIRECTION_LEFT;
            }
            if(input->keyboard[SCANCODE_D]) {
                movement.x += 1.0f;
                if(game_data->attack_time <= 0.0f)
                    game_data->player_direction = DIRECTION_RIGHT;
            }

            movement = vec3_fmul(movement, delta_time * move_speed);
            player->transform.translation = vec3_add(player->transform.translation, movement);
        
            // Move the cam
            Mat4 cam;
            Vec3 camera_offset = {0.0f, 10.0f, 5.0f};
            game_data->camera.position = vec3_add(player->transform.translation, camera_offset);
            game_data->camera.forward  = vec3_normalize(vec3_sub(game_data->camera.position, player->transform.translation));
            mat4_look_at(player->transform.translation, game_data->camera.position, (Vec3){0.0f, 1.0f, 0.0f}, cam);
            RendererSetCamera(global_renderer, cam, game_data->camera.position);

            // Attack
            if (InputKeyPressed(input, SCANCODE_SPACE)) {
                // Do attack
                game_data->attack_time = 0.1f;
                switch(game_data->player_direction) {
                    case DIRECTION_UP: {
                        game_data->sword_start_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, PI); 
                        game_data->sword_end_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, -HALF_PI); 
                        game_data->sword_offset = (Vec3) {0.5f, 0.5f, 0.0f};
                    } break;
                    case DIRECTION_DOWN: {
                        game_data->sword_start_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, 0.0f); 
                        game_data->sword_end_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, HALF_PI); 
                        game_data->sword_offset = (Vec3) {-0.5f, 0.5f, 0.5f};
                    } break;
                    case DIRECTION_LEFT: {
                        game_data->sword_start_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, -HALF_PI); 
                        game_data->sword_end_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, 0.0f); 
                        game_data->sword_offset = (Vec3) {-0.5f, 0.5f, -0.5f};
                    } break;
                    case DIRECTION_RIGHT: {
                        game_data->sword_start_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, HALF_PI); 
                        game_data->sword_end_rot = quat_from_axis((Vec3){0.0f, 1.0f, 0.0f}, PI); 
                        game_data->sword_offset = (Vec3) {0.5f, 0.5f, 0.5f};
                    } break;
                }
            }

            if(game_data->attack_time > 0.0f) {
                game_data->attack_time -= delta_time;
                Entity *sword = WorldGetEntity(&game_data->world, game_data->sword);
                sword->transform.rotation = quat_slerp(game_data->sword_end_rot, game_data->sword_start_rot, game_data->attack_time / 0.1f ); 
                sword->transform.translation = vec3_add(player->transform.translation, game_data->sword_offset);
                Vec3 forward = vec3_fmul(VEC3_FORWARD, 1.0f);
                forward = vec3_rotate(forward, sword->transform.rotation); 
                Vec3 sword_tip = vec3_add(forward, sword->transform.translation); 
                DebugPushPosition(&global_renderer->debug_pushbuffer, sword_tip); 
                DebugPushPosition(&global_renderer->debug_pushbuffer, sword->transform.translation); 
                PushMesh(&global_renderer->scene_pushbuffer, sword->static_mesh, &sword->transform, (Vec3){0.0f, 0.0f, 0.0f});

                Entity *enemy = WorldGetEntity(&game_data->world, game_data->enemy);
                if(game_data->enemy_health > 0) {
                    if(IsLineIntersectingBoundingBox(sword->transform.translation, sword_tip, &enemy->transform)) { 
                        if(!game_data->enemy_collided) {
                            game_data->enemy_collided = true;
                            game_data->enemy_health--;
                            sLog("Aie %d", game_data->enemy_health);
                        }
                    } else {
                        game_data->enemy_collided = false;
                    }
                }
            }

        } else {
            FPSCamera(&game_data->camera, input, true);
        }
        
        // Move the sun
        if(input->keyboard[SCANCODE_P]) {
            game_data->cos += delta_time;
            game_data->light_dir.x = cos(game_data->cos);
            game_data->light_dir.y = sin(game_data->cos);
            
            if(game_data->cos > 2.0f * PI) {
                game_data->cos = 0.0f;
            }
        }
        
        if(input->keyboard[SCANCODE_O]) {
            game_data->cos -= delta_time;
            game_data->light_dir.x = cos(game_data->cos);
            game_data->light_dir.y = sin(game_data->cos);
            if(game_data->cos < 0.0f) {
                game_data->cos = 2.0f * PI;
            }
        }
        RendererSetSunDirection(global_renderer, game_data->light_dir, vec3_add(player->transform.translation, (Vec3){0.0f, 0.0f, -7.0f}));
    }
    
    
    // -------------
    // Drawing
    
    DrawConsole(&game_data->console, game_data);
    
    { // NPC
        NPC *npc = &game_data->npc;
        Entity *e = WorldGetEntity(&game_data->world, npc->entity);
        
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
        PushSkinnedMesh(&global_renderer->scene_pushbuffer, e->skinned_mesh, &e->transform, e->skeleton, (Vec3){1.0f, 1.0f, 1.0f});
        
    }
    
    // Floor
    Entity *e = WorldGetEntity(&game_data->world, game_data->ground);
    PushMesh(&global_renderer->scene_pushbuffer, game_data->mesh_quad, &e->transform, (Vec3){0.8f, 0.8f, 0.8f});
    // Player
    e = WorldGetEntity(&game_data->world, game_data->player);
    PushMesh(&global_renderer->scene_pushbuffer, game_data->mesh_cube, &e->transform, (Vec3){1.0f, 1.0f, 1.0f});
    // Enemy
    e = WorldGetEntity(&game_data->world, game_data->enemy);
    PushMesh(&global_renderer->scene_pushbuffer, game_data->mesh_cube, &e->transform, (Vec3){1.0f, 0.0f, 0.0f});

    if(game_data->show_shadowmap)
        UIPushTexture(&global_renderer->ui_pushbuffer, global_renderer->backend->shadowmap_pass.texture, 0, global_renderer->height - 200, 200, 200);
    
}
