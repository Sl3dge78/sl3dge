#include <stdint.h>
#include <stdlib.h>

#define SL3DGE_IMPLEMENTATION
#include <sl3dge-utils/sl3dge.h>

#include "game.h"
#include "platform/platform.h"
#include "renderer/renderer.c"

global Renderer *global_renderer;
global PlatformAPI *platform;

#include "console.c"
#include "event.c"

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

    Leak_SetList(platform_api->DebugInfo);
}

/// This is called ONCE before the first frame
void GameStart(GameData *game_data) {
    sLogSetCallback(&ConsoleLogMessage);
    game_data->light_dir = (Vec3){1.0f, 1.0f, 0.0f};
    game_data->camera.position = (Vec3){0.0f, 1.8f, 0.0f};
    game_data->cos = 0;
    RendererSetSunDirection(global_renderer, vec3_normalize(vec3_fmul(game_data->light_dir, -1.0)));

    platform->SetCaptureMouse(true);

    game_data->floor = LoadQuad(global_renderer);
    game_data->floor_xform = mat4_identity();
    mat4_scaleby(&game_data->floor_xform, (Vec3){100.0f, 1.0f, 100.0f});

    game_data->npc = LoadCube(global_renderer);
    game_data->npc_xform = mat4_identity();
    mat4_scaleby(&game_data->npc_xform, (Vec3){1.0f, 2.0f, 1.0f});
    mat4_translate(&game_data->npc_xform, (Vec3){0.0f, 0.0f, 5.0f});
}

/// Do deallocation here
void GameEnd(GameData *game_data) {
    sFree(game_data->event_queue.queue);
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

    Vec3 forward = spherical_to_carthesian(camera->spherical_coordinates);
    Vec3 right = vec3_cross(forward, (Vec3){0.0f, 1.0f, 0.0f});

    // --------------
    // Move

    f32 move_speed = 0.1f;
    Vec3 movement = {0};

    if(input->keyboard[SCANCODE_LSHIFT]) {
        move_speed *= 5.0f;
    }

    if(input->keyboard[SCANCODE_W]) {
        movement = vec3_add(movement, vec3_fmul(forward, move_speed));
    }
    if(input->keyboard[SCANCODE_S]) {
        movement = vec3_add(movement, vec3_fmul(forward, -move_speed));
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
        camera->position.y = 1.8f;
    }

    Mat4 cam = mat4_look_at(vec3_add(forward, camera->position), camera->position, (Vec3){0.0f, 1.0f, 0.0f});

    RendererSetCamera(global_renderer, &cam, camera->position);
}

// Called every frame
void GameLoop(float delta_time, GameData *game_data, Input *input) {
    // ----------
    // Input

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

            RendererSetSunDirection(
                global_renderer, game_data->light_dir);
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
        default:

            break;
        };
    }

    // -------------
    // Drawing

    UIPushTexture(global_renderer, 0, 0, 0, 100, 100);
    char value[32];
    sprintf(value, "%f", game_data->cos);
    UIPushText(global_renderer, value, 0, 120, (Vec4){1.0f, 1.0f, 1.0f, 1.0f});
    sprintf(value, "%f %f %f", game_data->light_dir.x, game_data->light_dir.y, game_data->light_dir.z);
    UIPushText(global_renderer, value, 0, 140, (Vec4){1.0f, 1.0f, 1.0f, 1.0f});

    DrawConsole(&game_data->console, game_data);

    PushMesh(global_renderer, game_data->floor, &game_data->floor_xform, (Vec3){0.5f, 0.5f, 0.5f});
    PushMesh(global_renderer, game_data->npc, &game_data->npc_xform, (Vec3){1.0f, 1.0f, 1.0f});

    RendererSetSunDirection(global_renderer, game_data->light_dir);
}

// @Clean - Just in case
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