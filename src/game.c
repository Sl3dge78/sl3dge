#include <stdint.h>
#include <stdlib.h>

#define SL3DGE_IMPLEMENTATION
#include <sl3dge-utils/sl3dge.h>

#include "game.h"
#include "renderer/renderer.h"
#include "console.c"
#include "event.c"

internal void UIPushQuad(PushBuffer *push_buffer, const u32 x, const u32 y, const u32 w, const u32 h, const Vec4 color) {
    ASSERT(push_buffer->size + sizeof(PushBufferEntryQuad) < push_buffer->max_size);
    PushBufferEntryQuad *entry = (PushBufferEntryQuad *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Quad;
    entry->l = x;
    entry->t = y;
    entry->r = w + x;
    entry->b = h + y;
    entry->colour = color;
    push_buffer->size += sizeof(PushBufferEntryQuad);
}

internal void UIPushText(PushBuffer *push_buffer, const char *text, const u32 x, const u32 y, const Vec4 color) {
    ASSERT(push_buffer->size + sizeof(PushBufferEntryText) < push_buffer->max_size);
    PushBufferEntryText *entry = (PushBufferEntryText *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Text;
    entry->text = text;
    entry->x = x;
    entry->y = y;
    entry->colour = color;
    push_buffer->size += sizeof(PushBufferEntryText);
}

internal void PushMesh(PushBuffer *push_buffer, MeshHandle mesh, Mat4 *transform) {
    ASSERT(push_buffer->size + sizeof(PushBufferEntryText) < push_buffer->max_size);
    PushBufferEntryMesh *entry = (PushBufferEntryMesh *)(push_buffer->buf + push_buffer->size);
    entry->type = PushBufferEntryType_Mesh;
    entry->mesh_handle = mesh;
    entry->transform = transform;

    push_buffer->size += sizeof(PushBufferEntryMesh);
}

DLL_EXPORT void GameLoad(GameData *game_data) {
    sLogSetCallback(&ConsoleLogMessage);
    ConsoleInit(&game_data->console);
    global_console = &game_data->console;
}

DLL_EXPORT void GameStart(GameData *game_data) {
    game_data->light_pos = (Vec3){1.0f, 1.0f, 0.0f};
    game_data->camera.position = (Vec3){0.0f, 0.0f, 0.0f};

    game_data->moto_xform = mat4_identity();
    game_data->bike_dir = 0.0f;
    game_data->bike_forward = (Vec3){0.0f, 0.0f, -1.0f};
    game_data->renderer_api.SetSunDirection(game_data->renderer, vec3_normalize(vec3_fmul(game_data->light_pos, -1.0)));

    //game_data->renderer_api.SetCamera(game_data->renderer, game_data->camera.position, (Vec3){0.0f, 0.0f, -1.0f}, (Vec3){0.0f, 1.0f, 0.0f});
}

void FreecamMovement(GameData *game_data, GameInput *input) {
    f32 look_speed = 0.01f;
    if(input->mouse & MOUSE_RIGHT) {
        if(input->mouse_delta_x != 0) {
            game_data->camera.spherical_coordinates.x += look_speed * input->mouse_delta_x;
        }
        if(input->mouse_delta_y != 0) {
            float new_rot = game_data->camera.spherical_coordinates.y + look_speed * input->mouse_delta_y;
            if(new_rot > -PI / 2.0f && new_rot < PI / 2.0f) {
                game_data->camera.spherical_coordinates.y = new_rot;
            }
        }
        game_data->platform_api.SetCaptureMouse(true);

    } else {
        game_data->platform_api.SetCaptureMouse(false);
    }
    Vec3 forward = spherical_to_carthesian(game_data->camera.spherical_coordinates);
    Vec3 right = vec3_cross(forward, (Vec3){0.0f, 1.0f, 0.0f});

    // --------------
    // Move

    f32 move_speed = 0.1f;
    Vec3 movement = {0};

    if(input->keyboard[SCANCODE_LSHIFT]) {
        move_speed *= 10.0f;
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
    if(input->keyboard[SCANCODE_Q]) {
        movement.y -= move_speed;
    }
    if(input->keyboard[SCANCODE_E]) {
        movement.y += move_speed;
    }

    game_data->camera.position = vec3_add(game_data->camera.position, movement);

    Mat4 cam = mat4_look_at(vec3_add(forward, game_data->camera.position), game_data->camera.position, (Vec3){0.0f, 1.0f, 0.0f});

    game_data->renderer_api.SetCamera(game_data->renderer, &cam);
}

DLL_EXPORT void GameLoop(float delta_time, GameData *game_data, GameInput *input) {
    if(game_data->is_free_cam) {
        FreecamMovement(game_data, input);
    } else {
        if(input->keyboard[SCANCODE_A]) {
            game_data->bike_dir -= delta_time;
            game_data->bank_angle -= delta_time * 1.5f;
        }
        if(input->keyboard[SCANCODE_D]) {
            game_data->bike_dir += delta_time;
            game_data->bank_angle += delta_time * 1.5f;
        }

        if(game_data->bank_angle < 0)
            game_data->bank_angle += delta_time;
        else if(game_data->bank_angle > 0) {
            game_data->bank_angle -= delta_time;
        }

        game_data->bike_forward.x = -sin(game_data->bike_dir - 3.14f);
        game_data->bike_forward.z = cos(game_data->bike_dir - 3.14f);

        game_data->moto_xform = trs_to_mat4((Vec3){0.0f, 0.0f, 0.0f}, (Vec3){0.0f, game_data->bike_dir, game_data->bank_angle}, (Vec3){1.0f, 1.0f, 1.0f});

        Vec3 flat_forward = vec3_normalize(game_data->bike_forward);
        Vec3 offset_z = vec3_fmul(flat_forward, -60.f);
        Vec3 offset_y = {0.0f, 50.f, 0.f};
        Vec3 offset = vec3_add(offset_z, offset_y);

        Mat4 cam_xform = mat4_look_at(vec3_add(game_data->bike_forward, offset), offset, (Vec3){0.0f, 1.0f, 0.0f});

        game_data->renderer_api.SetCamera(game_data->renderer, &cam_xform);
    }

    // ----------------
    // Other inputs

    // Reset
    if(input->keyboard[SCANCODE_SPACE]) {
        //game_data->position = (Vec3){0.0f, 0, 0};
        //game_data->moto_xform = mat4_identity();
        mat4_translate(&game_data->moto_xform, (Vec3){0.0f, 0.0f, delta_time * 2.0f});
    }
    if(input->keyboard[SCANCODE_P]) {
        game_data->cos += delta_time;
        game_data->light_pos.x = cos(game_data->cos);
        game_data->light_pos.y = sin(game_data->cos);

        if(game_data->cos > PI) {
            game_data->cos = 0.0f;
        }
        game_data->renderer_api.SetSunDirection(
            game_data->renderer, vec3_normalize(vec3_fmul(game_data->light_pos, -1.0)));
    }
    if(input->keyboard[SCANCODE_O]) {
        game_data->cos -= delta_time;
        game_data->light_pos.x = cos(game_data->cos);
        game_data->light_pos.y = sin(game_data->cos);
        if(game_data->cos < 0.0f) {
            game_data->cos = PI;
        }

        game_data->renderer_api.SetSunDirection(
            game_data->renderer, vec3_normalize(vec3_fmul(game_data->light_pos, -1.0)));
    }

    // --------------
    // Console

    if(input->keyboard[SCANCODE_TILDE] && !input->old_keyboard[SCANCODE_TILDE]) {
        if(game_data->console.console_target <= 0) { // Console is closed, open it
            game_data->console.console_target = 300;
            input->read_text_input = true;
        }
        // The closing of the console is handled in the console, as it grabs all input.
    }
    DrawConsole(&game_data->console, game_data);
    if(game_data->console.console_open) {
        InputConsole(&game_data->console, input, game_data);
    }

    PushMesh(game_data->scene_push_buffer, 0, &game_data->moto_xform);
}
