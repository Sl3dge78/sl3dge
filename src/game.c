#include <stdint.h>
#include <stdlib.h>

#define SL3DGE_IMPLEMENTATION
#include <sl3dge-utils/sl3dge.h>

#include "game.h"
#include "renderer/renderer.h"
#include "console.c"
#include "event.c"

internal void UIPushQuad(PushBuffer *push_buffer,
                         const u32 x,
                         const u32 y,
                         const u32 w,
                         const u32 h,
                         const Vec4 color) {
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

internal void
UIPushText(PushBuffer *push_buffer, const char *text, const u32 x, const u32 y, const Vec4 color) {
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
    global_console = &game_data->console;
}

DLL_EXPORT void GameStart(GameData *game_data) {
    ConsoleInit();

    game_data->light_pos = (Vec3){1.0f, 1.0f, 0.0f};
    game_data->position = (Vec3){0.0f, 0.0f, 0.0f};

    /*
    game_data->cos = 0.0f;
    game_data->light_pos.x = cos(game_data->cos);
    game_data->light_pos.y = sin(game_data->cos);
    */
    game_data->moto_xform = mat4_identity();
    game_data->renderer_api.SetSunDirection(game_data->renderer,
                                            vec3_normalize(vec3_fmul(game_data->light_pos, -1.0)));
    //game_data->renderer_api.LoadMesh(game_data->renderer, "resources/models/gltf_samples/Sponza/glTF/Sponza.gltf");
    //game_data->renderer_api.LoadMesh(game_data->renderer,                                     "resources/3d/Motorcycle/motorcycle.gltf");
    //game_data->moto = game_data->renderer_api.InstantiateMesh(game_data->renderer, 0);
}

Vec3 FreecamMovement(GameInput *input, Vec3 forward, Vec3 right) {
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

    return movement;
}

DLL_EXPORT void GameLoop(float delta_time, GameData *game_data, GameInput *input) {
    // -----------
    // Look

    f32 look_speed = 0.01f;
    if(input->mouse & MOUSE_RIGHT) {
        if(input->mouse_delta_x != 0) {
            game_data->spherical_coordinates.x += look_speed * input->mouse_delta_x;
        }
        if(input->mouse_delta_y != 0) {
            float new_rot = game_data->spherical_coordinates.y + look_speed * input->mouse_delta_y;
            if(new_rot > -PI / 2.0f && new_rot < PI / 2.0f) {
                game_data->spherical_coordinates.y = new_rot;
            }
        }
        game_data->platform_api.SetCaptureMouse(true);

    } else {
        game_data->platform_api.SetCaptureMouse(false);
    }
    Vec3 forward = spherical_to_carthesian(game_data->spherical_coordinates);
    Vec3 right = vec3_cross(forward, (Vec3){0.0f, 1.0f, 0.0f});

    // --------------
    // Move

    if(game_data->is_free_cam) {
        game_data->position = vec3_add(game_data->position, FreecamMovement(input, forward, right));
        game_data->renderer_api.SetCamera(
            game_data->renderer, game_data->position, forward, (Vec3){0.0f, 1.0f, 0.0f});
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

    if(input->keyboard[SCANCODE_TILDE] && !input->old_keyboard[SCANCODE_TILDE]) {
        if(game_data->console.console_target <= 0) {
            game_data->console.console_target = 300;
            input->read_text_input = true;
        } else {
            game_data->console.console_target = 0;
            input->read_text_input = false;
        }
    }

    DrawConsole(&game_data->console, game_data);
    if(game_data->console.console_open) {
        InputConsole(&game_data->console, input, game_data);
    }

    PushMesh(game_data->scene_push_buffer, 0, &game_data->moto_xform);

#if 0
    mat4_rotate_euler(game_data->moto.transform, Vec3{0, game_data->spherical_coordinates.x, 0});
    mat4_set_position(game_data->moto.transform, game_data->position);
    Vec3 flat_forward = {forward.x, 0, forward.z};
    flat_forward = vec3_normalize(flat_forward);
    Vec3 offset_z = flat_forward * -10.f;
    Vec3 offset_y = {0.0f, 40.f, 0.f};
    Vec3 offset = offset_z + offset_y;

    game_data->renderer_api.SetCamera(
    game_data->renderer, game_data->position + offset, forward, Vec3{0.0f, 1.0f, 0.0f});
#endif
}
