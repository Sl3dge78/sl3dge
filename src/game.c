#include <stdint.h>
#include <stdlib.h>

#include <sl3dge-utils/sl3dge.h>

#include "game.h"
#include "renderer/renderer.h"

DLL_EXPORT void GameStart(GameData *game_data) {
    game_data->light_pos = (Vec3){1.0f, 1.0f, 0.0f};
    game_data->position = (Vec3){0.0f, 0.0f, 0.0f};
    /*
    game_data->cos = 0.0f;
    game_data->light_pos.x = cos(game_data->cos);
    game_data->light_pos.y = sin(game_data->cos);
    */
    game_data->renderer_api.SetSunDirection(game_data->renderer,
                                            vec3_normalize(vec3_fmul(game_data->light_pos, -1.0)));
    //game_data->renderer_api.LoadMesh(game_data->renderer, "resources/models/gltf_samples/Sponza/glTF/Sponza.gltf");
    //game_data->renderer_api.LoadMesh(game_data->renderer,                                     "resources/3d/Motorcycle/motorcycle.gltf");
    //game_data->moto = game_data->renderer_api.InstantiateMesh(game_data->renderer, 0);
}

DLL_EXPORT void GameLoop(float delta_time, GameData *game_data, GameInput *input) {
    f32 move_speed = 0.1f;
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
    //Vec3 up = vec3_cross(right, forward);

    Vec3 movement = {0};

    if(input->keyboard[SCANCODE_LSHIFT] & KEY_PRESSED) {
        move_speed *= 10.0f;
    }

    if(input->keyboard[SCANCODE_W] & KEY_PRESSED) {
        movement = vec3_add(movement, vec3_fmul(forward, move_speed));
    }
    if(input->keyboard[SCANCODE_S] & KEY_PRESSED) {
        movement = vec3_add(movement, vec3_fmul(forward, -move_speed));
    }
    if(input->keyboard[SCANCODE_A] & KEY_PRESSED) {
        movement = vec3_add(movement, vec3_fmul(right, -move_speed));
    }
    if(input->keyboard[SCANCODE_D] & KEY_PRESSED) {
        movement = vec3_add(movement, vec3_fmul(right, move_speed));
    }
    if(input->keyboard[SCANCODE_Q] & KEY_PRESSED) {
        movement.y -= move_speed;
    }
    if(input->keyboard[SCANCODE_E] & KEY_PRESSED) {
        movement.y += move_speed;
    }

    // Reset
    if(input->keyboard[SCANCODE_SPACE] & KEY_PRESSED) {
        // GameStart(game_data);
        game_data->position = (Vec3){0.0f, 0, 0};
        *game_data->moto.transform = mat4_identity();
    }

    if(input->keyboard[SCANCODE_P] & KEY_PRESSED) {
        game_data->cos += delta_time;
        game_data->light_pos.x = cos(game_data->cos);
        game_data->light_pos.y = sin(game_data->cos);

        if(game_data->cos > PI) {
            game_data->cos = 0.0f;
        }
        game_data->renderer_api.SetSunDirection(
            game_data->renderer, vec3_normalize(vec3_fmul(game_data->light_pos, -1.0)));
    }
    if(input->keyboard[SCANCODE_O] & KEY_PRESSED) {
        game_data->cos -= delta_time;
        game_data->light_pos.x = cos(game_data->cos);
        game_data->light_pos.y = sin(game_data->cos);
        if(game_data->cos < 0.0f) {
            game_data->cos = PI;
        }
        game_data->renderer_api.SetSunDirection(
            game_data->renderer, vec3_normalize(vec3_fmul(game_data->light_pos, -1.0)));
    }

    if(input->keyboard[SCANCODE_M] & KEY_PRESSED) {
        game_data->renderer_api.InstantiateMesh(game_data->renderer, 0);
    }

    game_data->position = vec3_add(game_data->position, movement);
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
    game_data->renderer_api.SetCamera(
        game_data->renderer, game_data->position, forward, (Vec3){0.0f, 1.0f, 0.0f});
}