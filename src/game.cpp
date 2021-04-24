#include "game.h"

/*
=== TODO ===

 CRITICAL

 MAJOR

 BACKLOG

 IMPROVEMENTS

 IDEAS
 
*/

extern "C" __declspec(dllexport) GAME_START(GameStart) {
    game_data->matrices.proj = mat4_perspective(90.0f, 1280.0f/720.0f, 0.01f, 100000.0f);
    game_data->light_pos = {0.0f,0.0f,0.0f};
}

extern "C" __declspec(dllexport) GAME_LOOP(GameLoop) {
    
    float speed = delta_time;
    
    i32 mouse_x;
    i32 mouse_y;
    u32 mouse_state = SDL_GetRelativeMouseState(&mouse_x, &mouse_y);
    
    if(mouse_state == SDL_BUTTON(3)) {
        if(mouse_x != 0) {
            game_data->spherical_coordinates.x += speed * mouse_x * 2.5f;
        }
        if(mouse_y != 0){
            float new_rot = game_data->spherical_coordinates.y + speed * mouse_y * 2.5f;
            if(new_rot > -PI/2.0f && new_rot < PI/2.0f) {
                game_data->spherical_coordinates.y = new_rot;
            }
        }
        SDL_SetRelativeMouseMode(SDL_TRUE);
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
    
    vec3 forward = spherical_to_carthesian(game_data->spherical_coordinates);
    vec3 right = vec3_cross(forward, vec3{0.0f, 1.0f, 0.0f});
    vec3 up = vec3_cross(right, forward);
    
    vec3 movement = {};
    const Uint8 *keyboard = SDL_GetKeyboardState(NULL);
    
    
    if(keyboard[SDL_SCANCODE_LSHIFT]){
        speed *= 100.0f;
    }
    
    if(keyboard[SDL_SCANCODE_W]) {
        movement = movement + vec3_fmul(forward, speed);
    }
    if(keyboard[SDL_SCANCODE_S]) {
        movement = movement + vec3_fmul(forward,-speed);
    }
    if(keyboard[SDL_SCANCODE_A]){
        movement = movement + vec3_fmul(right, -speed);
    }
    if(keyboard[SDL_SCANCODE_D]){
        movement = movement + vec3_fmul(right, speed);
    }
    if(keyboard[SDL_SCANCODE_Q]){
        movement.y -= speed;
    }
    if(keyboard[SDL_SCANCODE_E]){
        movement.y += speed;
    }
    
    // Reset
    if(keyboard[SDL_SCANCODE_SPACE]){
        //GameStart(game_data);
        game_data->cos = 0.0f;
    }
    
    if(keyboard[SDL_SCANCODE_P]) {
        game_data->cos += speed;
        game_data->light_pos.x = cos(game_data->cos);
        game_data->light_pos.y = sin(game_data->cos);
        
        if(game_data->cos > PI) {
            game_data->cos = 0.0f;
        }
    }
    if(keyboard[SDL_SCANCODE_O]) {
        game_data->cos -= speed;
        game_data->light_pos.x = cos(game_data->cos);
        game_data->light_pos.y = sin(game_data->cos);
        if(game_data->cos < 0.0f) {
            game_data->cos = PI;
        }
    }
    
    game_data->position = game_data->position + movement;
    game_data->matrices.pos = game_data->position;
    game_data->matrices.view = mat4_look_at(game_data->position + forward, game_data->position, vec3{0.0f, 1.0f, 0.0f} );
    
    mat4 a = mat4_ortho_zoom(1.0f/1.0f, 250.0f, -600.0f, 600.0f);
    mat4 b = mat4_look_at({0.0f, 0.0f,0.0f }, game_data->light_pos, vec3{0.0f, 1.0f, 0.0f});
    
    game_data->matrices.shadow_mvp = mat4_mul(&a, &b);
    game_data->matrices.light_dir = vec3_normalize(game_data->light_pos * -1.0);
}