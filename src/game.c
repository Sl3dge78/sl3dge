#include <stdint.h>
#include <stdlib.h>

#include "utils/sl3dge.h"
#include "platform/platform.h"

#include "utils/sGltf.c"

#include "renderer/pushbuffer.h"
#include "renderer/renderer.h"

#include "console.h"
#include "event.h"
#include "entities.h"
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
#include "entities.c"
#include "world.c"

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
}

/// This is called ONCE before the first frame. Won't be called upon reloading.
DLL_EXPORT void GameStart(GameData *game_data) {
    
    WorldInit(&game_data->world, 128);
    game_data->light_dir = (Vec3){0.67f, -0.67f, 0.1f};
    game_data->cos = 0;
    RendererSetSunDirection(global_renderer, vec3_normalize(vec3_fmul(game_data->light_dir, -1.0)));
    
    platform->SetCaptureMouse(true);
    
    game_data->mesh_quad = LoadQuad(global_renderer);
    game_data->mesh_cube = LoadCube(global_renderer);

    // @Todo : maybe this shouldn't be an entity?
    for(i32 x = -5; x < 5; x++) {
        for(i32 y = -5; y < 5; y++) {
            Entity *floor_entity;
            game_data->ground = WorldCreateAndGetEntity(&game_data->world, &floor_entity);
            floor_entity->transform.scale = (Vec3){0.9f, 1.0f, 0.9f};
            floor_entity->transform.translation = (Vec3){x, 0.0f, y};
            floor_entity->static_mesh = game_data->mesh_quad;
            floor_entity->color = (Vec3){0.5f, 0.7f, 0.5f};
            floor_entity->flags |= EntityFlag_Sleeping;
        }
    }
    
    game_data->player = CreatePlayer(&game_data->world, game_data->mesh_cube);

    game_data->enemy_count = 5;
    game_data->enemies = sCalloc(game_data->enemy_count, sizeof(EntityID));
    for(u32 i = 0; i < game_data->enemy_count; i++) {
        game_data->enemies[i] = CreateEnemy(&game_data->world, game_data->mesh_cube);
    }
    game_data->sword = CreateSword(&game_data->world);

    CreateNPC(&game_data->world, global_renderer, platform, &game_data->npc);
}

/// Do deallocation here
DLL_EXPORT void GameEnd(GameData *game_data) {
    sFree(game_data->enemies);
    if(game_data->event_queue.queue) {
        sFree(game_data->event_queue.queue);
    }
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

    // -------------
    // Drawing
    
    DrawConsole(&game_data->console, game_data);
    
    Input *given_input = NULL;
    if(game_data->console.console_open) {
        InputConsole(&game_data->console, input, game_data);
    } else {
        if(!game_data->is_free_cam)
            given_input = input;
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
        RendererSetSunDirection(global_renderer, game_data->light_dir);
    }

    UpdateAndDrawEntities(game_data, &game_data->world, given_input, global_renderer, delta_time);

    // Camera
    if(!game_data->console.console_open) {
        if(!game_data->is_free_cam) {
            Entity *player = WorldGetEntity(&game_data->world, game_data->player);
            Mat4 cam;
            Vec3 camera_offset = {0.0f, 10.0f, 5.0f};
            game_data->camera.position = vec3_add(player->transform.translation, camera_offset);
            game_data->camera.forward  = vec3_normalize(vec3_sub(game_data->camera.position, player->transform.translation));
            mat4_look_at(player->transform.translation, game_data->camera.position, (Vec3){0.0f, 1.0f, 0.0f}, cam);
            RendererSetCamera(global_renderer, cam, game_data->camera.position);
        } else {
            FPSCamera(&game_data->camera, input, true);
        }
    }   


    if(game_data->show_shadowmap)
        UIPushTexture(&global_renderer->ui_pushbuffer, global_renderer->backend->shadowmap_pass.texture, 0, global_renderer->height - 200, 200, 200);
    
}
