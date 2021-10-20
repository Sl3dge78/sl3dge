#include <stdint.h>
#include <stdlib.h>

#include "utils/sl3dge.h"

#include "game.h"
#include "platform/platform.h"
#include "renderer/renderer.c"

global Renderer *global_renderer;
global PlatformAPI *platform;

#include "console.c"
#include "event.c"

#include "collision.h"

u32 GameGetSize() {
    return sizeof(GameData);
}


// ---------------
// Exported functions

/// This is called when the game is (re)loaded
void GameLoad(GameData *game_data, Renderer *renderer, PlatformAPI *platform_api) {
    global_renderer = renderer;
    platform = platform_api;
    
    GLLoadFunctions();
    
    ConsoleInit(&game_data->console);
    global_console = &game_data->console;
    sLogSetCallback(&ConsoleLogMessage);
    
    Leak_SetList(platform_api->DebugInfo);
}

/// This is called ONCE before the first frame
void GameStart(GameData *game_data) {
    
    game_data->light_dir = (Vec3){0.67f, -0.67f, 0.1f};
    game_data->camera.position = (Vec3){0.0f, 1.7f, 0.0f};
    game_data->cos = 0;
    RendererSetSunDirection(global_renderer, vec3_normalize(vec3_fmul(game_data->light_dir, -1.0)));
    
    platform->SetCaptureMouse(true);
    game_data->box = RendererLoadCube(global_renderer);
    
    game_data->character = RendererLoadMesh(global_renderer, "resources/3d/character/character.gltf");
    
    game_data->floor = RendererLoadQuad(global_renderer);
    game_data->floor_xform = RendererAllocateTransforms(global_renderer, 1);
    game_data->floor_xform->scale = (Vec3){100.0f, 1.0f, 100.0f};
    
    game_data->npc_xform = RendererAllocateTransforms(global_renderer, 1);
    game_data->npc_xform->translation.z = 5.0f;
    
    game_data->interact_sphere_diameter = 1.0f;
    
    game_data->skinned_mesh = RendererLoadSkinnedMesh(global_renderer, "resources/3d/skintest.gltf");
    game_data->simple_skinning_root = RendererAllocateTransforms(global_renderer, 1);
    
    {
        game_data->anim_time = 0.0f;
        
        Animation *a = &game_data->anim;
        a->key_count = 3;
        a->key_times = sCalloc(a->key_count, sizeof(f32));
        a->quat_keys = sCalloc(a->key_count, sizeof(Quat));
        
        a->quat_keys[0] = quat_from_axis((Vec3){0.0f,0.0f,1.0f}, 0.0f);
        a->key_times[0] = 0.0f;
        a->quat_keys[1] = quat_from_axis((Vec3){0.0f,0.0f,1.0f}, 1.14f);
        a->key_times[1] = 1.0f;
        a->quat_keys[2] = quat_from_axis((Vec3){0.5f,0.0f,0.5f}, -3.14f);
        a->key_times[2] = 2.0f;
        
        a->length = a->key_times[a->key_count - 1];
    }
}

/// Do deallocation here
void GameEnd(GameData *game_data) {
    sFree(game_data->event_queue.queue);
    sFree(game_data->anim.key_times);
    sFree(game_data->anim.quat_keys);
}

void FPSCamera(Camera *camera, Input *input, bool is_free_cam) {
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


Quat AnimationEvaluate(Animation *a, f32 time) {
    
    u32 key_1 = 0; u32 key_2 = 0;
    
    // clamp time
    if (time > a->length)
        time = a->length;
    
    for(u32 i = 0; i < a->key_count; i ++) {
        if(time > a->key_times[i] && time < a->key_times[i + 1]) {
            key_1 = i;
            key_2 = i + 1;
            
            break;
        }
    }
    ASSERT(key_1 != key_2);
    
    f32 time_between_keys = a->key_times[key_2] - a->key_times[key_1];
    f32 rel_t = time - a->key_times[key_1];
    f32 norm_t = rel_t / time_between_keys;
    
    return(quat_slerp(a->quat_keys[key_1], a->quat_keys[key_2], norm_t));
}

// Called every frame
void GameLoop(float delta_time, GameData *game_data, Input *input) {
    // ----------
    // Input
    
    // Console
    if(input->keyboard[SCANCODE_TILDE] && !input->old_keyboard[SCANCODE_TILDE]) {
        if(game_data->console.console_target <= 0) { // Console is closed, open it
            game_data->console.console_target = 300;
            input->read_text_input = true;
            platform->SetCaptureMouse(false);
        } else { // Console is opened, close it
            game_data->console.console_target = 0;
            input->read_text_input = false;
            platform->SetCaptureMouse(true);
        }
    }
    
    if(game_data->console.console_open) {
        InputConsole(&game_data->console, input, game_data);
    } else {
        FPSCamera(&game_data->camera, input, game_data->is_free_cam);
        
        // Interact sphere
        game_data->interact_sphere_pos = vec3_add(game_data->camera.position, game_data->camera.forward);
        
        if(input->keyboard[SCANCODE_E] && !input->old_keyboard[SCANCODE_E]) {
            if(IsLineIntersectingBoundingBox(game_data->camera.position, game_data->interact_sphere_pos, game_data->npc_xform)) {
                game_data->npc_xform->rotation.y += 1.0f;
            }
        }
        
        // Move the sun
        if(input->keyboard[SCANCODE_P]) {
            game_data->cos += delta_time;
            game_data->light_dir.x = cos(game_data->cos);
            game_data->light_dir.y = sin(game_data->cos);
            
            if(game_data->cos > 2.0f * PI) {
                game_data->cos = 0.0f;
            }
            RendererSetSunDirection(
                                    global_renderer, game_data->light_dir);
        }
        
        if(input->keyboard[SCANCODE_O]) {
            game_data->cos -= delta_time;
            game_data->light_dir.x = cos(game_data->cos);
            game_data->light_dir.y = sin(game_data->cos);
            if(game_data->cos < 0.0f) {
                game_data->cos = 2.0f * PI;
            }
            
            RendererSetSunDirection(global_renderer, game_data->light_dir);
        }
        
        if(input->keyboard[SCANCODE_Y]) {
            game_data->skinned_mesh->joints[2].rotation = quat_identity();
        }
        if(input->keyboard[SCANCODE_U]) {
            game_data->skinned_mesh->joints[2].rotation = quat_from_axis((Vec3){1.0f, 0.0f, 0.0f}, 90.f);
        }
    }
    
    // --------------
    // Event
    
    EventType e;
    while(EventConsume(&game_data->event_queue, &e)) {
        switch(e) {
            case(EVENT_TYPE_QUIT): {
                platform->RequestExit();
            } break;
            case(EVENT_TYPE_RESTART): {
                GameStart(game_data);
            } break;
            case(EVENT_TYPE_RELOADSHADERS): {
                RendererReloadShaders(global_renderer, platform);
            } break;
            default:
            
            break;
        };
    }
    
    // -------------
    // Drawing
    
    DrawConsole(&game_data->console, game_data);
    { // Animation
        
        game_data->anim_time = fmod(game_data->anim_time + delta_time, game_data->anim.length);
        game_data->skinned_mesh->joints[2].rotation = AnimationEvaluate(&game_data->anim, game_data->anim_time);
    }
    
    PushMesh(global_renderer, game_data->floor, game_data->floor_xform, (Vec3){0.5f, 0.5f, 0.5f});
    PushMesh(global_renderer, game_data->character, game_data->npc_xform, (Vec3){1.0f, 1.0f, 1.0f});
    PushSkinnedMesh(global_renderer, game_data->skinned_mesh, game_data->simple_skinning_root, (Vec3){1.0f, 1.0f, 1.0f});
    
    for (u32 i = 0; i < 4; i++) {
        PushBone(global_renderer, game_data->skinned_mesh->global_joint_mats[i]);
    }
    
    RendererSetSunDirection(global_renderer, game_data->light_dir);
}

// @Clean - Kept just in case
/*else {
        if(input->keyboard[SCANCODE_A]) {
            //game_data->bike_dir -= delta_time;
            game_data->bank_angle -= delta_time * 1.5f;
            if(game_data->bank_angle < -0.5f) {
                game_data->bank_angle = -0.5f;
            }
            game_data->bike_x -= delta_time * 50.0f;
        }
        if(input->keyboard[SCANCODE_D]) {
            //game_data->bike_dir += delta_time;
            game_data->bank_angle += delta_time * 1.5f;
            if(game_data->bank_angle > 0.5f) {
                game_data->bank_angle = 0.5f;
            }
            game_data->bike_x += delta_time * 50.0f;
        }

        if(game_data->bank_angle < 0) {
            game_data->bank_angle += delta_time;
        } else if(game_data->bank_angle > 0) {
            game_data->bank_angle -= delta_time;
        }

        game_data->bike_forward.x = -sin(game_data->bike_dir - 3.14f);
        game_data->bike_forward.z = cos(game_data->bike_dir - 3.14f);

        game_data->moto_xform = trs_to_mat4((Vec3){game_data->bike_x, 0.0f, 0.0f}, (Vec3){0.0f, game_data->bike_dir, game_data->bank_angle}, (Vec3){1.0f, 1.0f, 1.0f});

        Vec3 flat_forward = vec3_normalize(game_data->bike_forward);
        Vec3 offset_z = vec3_fmul(flat_forward, -60.f);
        Vec3 offset_y = {0.0f, 50.f, 0.f};
        Vec3 offset = vec3_add(offset_z, offset_y);
        offset.x += game_data->bike_x;

        Mat4 cam_xform = mat4_look_at(vec3_add(game_data->bike_forward, offset), offset, (Vec3){0.0f, 1.0f, 0.0f});

        RendererSetCamera(global_renderer, &cam_xform);
    }*/