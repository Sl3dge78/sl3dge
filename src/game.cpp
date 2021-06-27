#include <stdint.h>
#include <stdlib.h>

#include <SDL/SDL.h>

#include <sl3dge/sl3dge.h>

#include "game.h"
#include "renderer/renderer.h"
/*
=== TODO ===

 CRITICAL

 MAJOR

 BACKLOG
- Moto movement
- Moto cam
- Generate roads

 IMPROVEMENTS

 IDEAS

*/

DLL_EXPORT void GameStart(GameData *game_data) {
    game_data->light_pos = {0.0f, 0.0f, 0.0f};
    game_data->position = {0.0f, 50.0f, 0.0f};

    game_data->renderer_api.LoadMesh(game_data->renderer,
                                     "resources/3d/Motorcycle/motorcycle.gltf");
    game_data->renderer_api.InstantiateMesh(game_data->renderer, 0);
}

DLL_EXPORT void GameLoop(float delta_time, GameData *game_data) {
    f32 move_speed = 1.0f;
    f32 look_speed = 0.01f;

    i32 mouse_x;
    i32 mouse_y;
    u32 mouse_state = SDL_GetRelativeMouseState(&mouse_x, &mouse_y);

    if(mouse_state == SDL_BUTTON(3)) {
        if(mouse_x != 0) {
            game_data->spherical_coordinates.x += look_speed * mouse_x;
        }
        if(mouse_y != 0) {
            float new_rot = game_data->spherical_coordinates.y + look_speed * mouse_y;
            if(new_rot > -PI / 2.0f && new_rot < PI / 2.0f) {
                game_data->spherical_coordinates.y = new_rot;
            }
        }
        SDL_SetRelativeMouseMode(SDL_TRUE);
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    Vec3 forward = spherical_to_carthesian(game_data->spherical_coordinates);
    Vec3 right = vec3_cross(forward, Vec3{0.0f, 1.0f, 0.0f});
    Vec3 up = vec3_cross(right, forward);

    Vec3 movement = {};
    const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

    if(keyboard[SDL_SCANCODE_LSHIFT]) {
        move_speed *= 10.0f;
    }

    if(keyboard[SDL_SCANCODE_W]) {
        movement = movement + vec3_fmul(forward, move_speed);
    }
    if(keyboard[SDL_SCANCODE_S]) {
        movement = movement + vec3_fmul(forward, -move_speed);
    }
    if(keyboard[SDL_SCANCODE_A]) {
        movement = movement + vec3_fmul(right, -move_speed);
    }
    if(keyboard[SDL_SCANCODE_D]) {
        movement = movement + vec3_fmul(right, move_speed);
    }
    if(keyboard[SDL_SCANCODE_Q]) {
        movement.y -= move_speed;
    }
    if(keyboard[SDL_SCANCODE_E]) {
        movement.y += move_speed;
    }

    // Reset
    if(keyboard[SDL_SCANCODE_SPACE]) {
        // GameStart(game_data);
        game_data->cos = 0.0f;
    }

    if(keyboard[SDL_SCANCODE_P]) {
        game_data->cos += delta_time;
        game_data->light_pos.x = cos(game_data->cos);
        game_data->light_pos.y = sin(game_data->cos);

        if(game_data->cos > PI) {
            game_data->cos = 0.0f;
        }
    }
    if(keyboard[SDL_SCANCODE_O]) {
        game_data->cos -= delta_time;
        game_data->light_pos.x = cos(game_data->cos);
        game_data->light_pos.y = sin(game_data->cos);
        if(game_data->cos < 0.0f) {
            game_data->cos = PI;
        }
    }

    game_data->position = game_data->position + movement;
    game_data->renderer_api.SetCamera(
        game_data->renderer, game_data->position, forward, Vec3{0.0f, 1.0f, 0.0f});

    game_data->renderer_api.SetSunDirection(game_data->renderer,
                                            vec3_normalize(game_data->light_pos * -1.0));
}