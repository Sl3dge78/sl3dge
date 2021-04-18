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
    
    game_data->spherical_coordinates = {0.0f, 0.0f };
    game_data->position = { 1.5f, 0.0f, 2.0f };
    game_data->matrices.proj = mat4_perspective(90.0f, 1280.0f/720.0f, 0.01f, 50.0f);
    //game_data->matrices.view = mat4_identity();
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
        speed *= 10.0f;
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
        //game_data->transforms[0] = mat4_identity();
        //game_data->transforms[1] = mat4_identity();
        GameStart(game_data);
    }
    
    game_data->position = game_data->position + movement;
    game_data->matrices.view = mat4_look_at(game_data->position + forward, game_data->position, vec3{0.0f, 1.0f, 0.0f} );
    game_data->matrices.view_dir = forward;
    game_data->matrices.pos = game_data->position;
    
}