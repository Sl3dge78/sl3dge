#include <stdint.h>
#include <stdlib.h>

#include "utils/sl3dge.h"
#include "platform/platform.h"

#include "utils/sGltf.c"

#include "renderer/pushbuffer.h"
#include "renderer/renderer.h"

#include "console.h"
#include "event.h"
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
}

/// This is called ONCE before the first frame. Won't be called upon reloading.
DLL_EXPORT void GameStart(GameData *game_data) {
    
    game_data->light_dir = (Vec3){0.67f, -0.67f, 0.1f};
    game_data->camera.position = (Vec3){0.0f, 1.7f, 0.0f};
    game_data->cos = 0;
    RendererSetSunDirection(global_renderer, vec3_normalize(vec3_fmul(game_data->light_dir, -1.0)));
    
    platform->SetCaptureMouse(true);
    
    game_data->floor = LoadQuad(global_renderer);
    game_data->floor_xform = AllocateTransforms(global_renderer, 1);
    Transform *f = ArrayGetElementAt(global_renderer->transforms, game_data->floor_xform);
    f->scale = (Vec3){100.0f, 1.0f, 100.0f};
    
    game_data->cube = LoadCube(global_renderer);
    game_data->cube_xform = AllocateTransforms(global_renderer, 1);
    Transform *c = ArrayGetElementAt(global_renderer->transforms, game_data->cube_xform);
    c->translation.x = 10.0f;
    
    
    LoadFromGLTF("resources/3d/skintest.gltf", global_renderer, platform, &game_data->cylinder_mesh, &game_data->cylinder_skin, 0);
    game_data->cylinder_xform = AllocateTransforms(global_renderer, 1);
    Transform *cyl = ArrayGetElementAt(global_renderer->transforms, game_data->cylinder_xform);
    cyl->translation.x = 0.0f;
    
    
    LoadFromGLTF("resources/3d/character/walk.gltf", global_renderer, platform, &game_data->character, &game_data->character_skin, &game_data->walk);
    game_data->character_xform = AllocateTransforms(global_renderer, 1);
    Transform *ch = ArrayGetElementAt(global_renderer->transforms, game_data->character_xform);
    ch->translation.z = 5.0f;
    game_data->anim_time = 0.0f;
    
    game_data->interact_sphere_diameter = 1.0f;
    
#if 0
    
    {
        Animation *a = &game_data->anim;
        a->track_count = 2;
        a->tracks = sCalloc(a->track_count, sizeof(AnimationTrack));
        {
            a->tracks[0].type = ANIM_TYPE_QUATERNION;
            a->tracks[0].key_count = 3;
            a->tracks[0].key_times = sCalloc(a->tracks[0].key_count, sizeof(f32));
            
            Quat *keys = sCalloc(a->tracks[0].key_count, sizeof(Quat));
            
            keys[0] = quat_from_axis((Vec3){0.0f,0.0f,1.0f}, 0.0f);
            a->tracks[0].key_times[0] = 0.0f;
            keys[1] = quat_from_axis((Vec3){0.0f,0.0f,1.0f}, 1.14f);
            a->tracks[0].key_times[1] = 1.0f;
            keys[2] = quat_from_axis((Vec3){0.5f,0.0f,0.5f}, -3.14f);
            a->tracks[0].key_times[2] = 3.0f;
            a->tracks[0].keys = keys;
            a->tracks[0].target = &game_data->skinned_mesh->joints[1].rotation;
            a->length = a->tracks[0].key_times[a->tracks[0].key_count - 1];
        }
        {
            a->tracks[1].type = ANIM_TYPE_VEC3;
            a->tracks[1].key_count = 3;
            a->tracks[1].key_times = sCalloc(a->tracks[1].key_count, sizeof(f32));
            
            Vec3 * keys = sCalloc(a->tracks[1].key_count, sizeof(Vec3));
            keys[0] = (Vec3){0.0f,0.0f,1.0f};
            a->tracks[1].key_times[0] = 0.0f;
            keys[1] = (Vec3){0.0f,0.0f,2.0f};
            a->tracks[1].key_times[1] = 1.0f;
            keys[2] = (Vec3){0.0f,1.0f,1.0f};
            a->tracks[1].key_times[2] = 2.0f;
            a->tracks[1].keys = keys;
            a->tracks[1].target = &game_data->skinned_mesh->joints[0].translation;
            if (a->length < a->tracks[1].key_times[a->tracks[1].key_count - 1]) 
                a->length = a->tracks[1].key_times[a->tracks[1].key_count - 1];
        }
        
    }
#endif
}

/// Do deallocation here
DLL_EXPORT void GameEnd(GameData *game_data) {
    sFree(game_data->event_queue.queue);
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

void DrawAnimationUI(PushBuffer *ui, Animation *animation, f32 time, u32 bone, u32 key) {
    u32 x = 10;
    u32 y = global_renderer->height - 80;
    u32 w = global_renderer->width - 40;
    u32 h = 70;
    
    // Panel
    PushUIQuad(ui, x, y, w, h, (Vec4){0.1f, 0.1f, 0.1f, 0.5f});
    
    // Text
    PushUIFmt(ui, x + 5, y + 20, (Vec4){1.0f, 1.0f, 1.0f, 1.0f}, "Current Time: %.2f, Key: %d, Bone: %d", time, key, bone);
    
    // Timeline
    PushUIQuad(ui, x + 5, y + 50, w - 10, 10, (Vec4){0.1f, 0.1f, 0.1f, 0.8f});
    
    AnimationTrack *track = &animation->tracks[0];
    
    // Keys
    for(u32 i = 0; i < track->key_count; i++) {
        f32 rel_t = track->key_times[i] / animation->length;
        Vec4 color = (Vec4){1.0f, 0.5f, 0.5f, 1.0f};
        if(key == i)
            color.y = 0.0f;
        PushUIQuad(ui, x + 5 + rel_t * (w - 10), y + 50, 1, 10, color);
    }
    
    // Playhead
    f32 rel_time = time / animation->length;
    PushUIQuad(ui, x + 5 + rel_time * w, y + 45, 1, 20, (Vec4){0.9f, 0.9f, 0.9f, 1.0f});
    
    
    Quat v1;
    Quat v2;
    u32 track_id = 0;
    for(u32 i = 0; i < animation->track_count; i ++){
        AnimationTrack *t = &animation->tracks[i];
        if(t->target == ANIM_TARGET_ROTATION && t->target_node == bone) {
            track_id = i;
            Quat *keys = (Quat *)(t->keys);
            v1 = keys[key];
            v2 = keys[key + 1];
            
            /*
            f32 time_between_keys = t->key_times[key + 1] - t->key_times[key];
            f32 rel_t = time - track->key_times[key];
            f32 norm_t = rel_t / time_between_keys;
            
            Quat q_lerp = quat_slerp(keys[key], keys[key + 1], norm_t);
            quat_sprint(q_lerp, qlerp);
            */
            break;
        }
    }
    PushUIFmt(ui, x + 5, y + 40, (Vec4){1.0f, 1.0f, 1.0f, 1.0f}, "%d 1: %f %f %f %f | 2: %f %f %f %f", track_id, v1.x, v1.y, v1.z, v1.w, v2.x, v2.y, v2.z, v2.w);
    //PushUIFmt(ui, global_renderer->width / 2 - 200, y + 40, (Vec4){1.0f, 1.0f, 1.0f, 1.0f}, "   %s", qlerp);
    
}

/// Called every frame
DLL_EXPORT void GameLoop(float delta_time, GameData *game_data, Input *input) {
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
            Transform *x = ArrayGetElementAt(global_renderer->transforms, game_data->character_xform);
            if(IsLineIntersectingBoundingBox(game_data->camera.position, game_data->interact_sphere_pos, x)) {
                
                x->rotation.y += 1.0f;
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
            RendererSetSunDirection(global_renderer, game_data->light_dir);
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
        
        if(input->keyboard[SCANCODE_U]){
            if(input->keyboard[SCANCODE_LSHIFT])
                game_data->anim_time += delta_time;
            else
                game_data->anim_time += 0.001f;
        }
        
        if(input->keyboard[SCANCODE_Y]){
            if(input->keyboard[SCANCODE_LSHIFT])
                game_data->anim_time -= delta_time;
            else
                game_data->anim_time -= 0.001f;
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
            case(EVENT_TYPE_RELOAD): {
                platform->RequestReload();
            } break;
            default:
            
            break;
        };
    }
    
    // -------------
    // Drawing
    
    DrawConsole(&game_data->console, game_data);
    
    { // Animation
        Skin *skin = ArrayGetElementAt(global_renderer->skins, game_data->character_skin);
        //Animation *animation = ArrayGetElementAt(global_renderer->animations, game_data->walk);
        
        //DrawAnimationUI(&global_renderer->ui_pushbuffer, animation, game_data->anim_time, game_data->bone, game_data->key);
        
        // game_data->anim_time = fmod(game_data->anim_time + delta_time, animation->length);
        AnimationEvaluate(global_renderer, skin->first_joint, skin->joint_count, game_data->walk, game_data->anim_time);
        
        for(u32 i = 0; i < skin->joint_count; i++) {
            PushBone(&global_renderer->debug_pushbuffer, skin->global_joint_mats[i]);
        }
    }
    
    PushMesh(&global_renderer->scene_pushbuffer, game_data->floor, game_data->floor_xform, (Vec3){0.5f, 0.5f, 0.5f});
    PushMesh(&global_renderer->scene_pushbuffer, game_data->cube, game_data->cube_xform, (Vec3){1.0f, 1.0f, 1.0f});
    PushSkin(&global_renderer->scene_pushbuffer, game_data->cylinder_mesh, game_data->cylinder_skin, game_data->cylinder_xform, (Vec3){1.0f, 1.0f, 1.0f});
    PushSkin(&global_renderer->scene_pushbuffer, game_data->character, game_data->character_skin, game_data->character_xform, (Vec3){1.0f, 1.0f, 1.0f});
    
    if(game_data->show_shadowmap)
        PushUITexture(&global_renderer->ui_pushbuffer, global_renderer->backend->shadowmap_pass.texture, 0, 0, 500, 500);
    
    RendererSetSunDirection(global_renderer, game_data->light_dir);
}