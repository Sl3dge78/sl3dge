#include "game.h"

#include <string.h>

/*
=== TODO ===

 CRITICAL

 MAJOR

 BACKLOG

 IMPROVEMENTS

 IDEAS
 
*/

extern "C" __declspec(dllexport) GAME_START(GameStart) {
    
    game_data->rotation = { 0.0f };
    game_data->position = { 0.0f };
    game_data->matrices.proj = mat4_perspective(90.0f, 1280.0f/720.0f, 0.0f, 1000.0f);
    game_data->matrices.view = mat4_identity();
    game_data->matrices.mesh = mat4_identity();
}

extern "C" __declspec(dllexport) GAME_GET_SCENE(GameGetScene) {
    
    const vec3 vtx[] = {
        {0.0f, 0.f, 0.0f}, {0.0f, 1.f, 0.0f},
        {1.0f, 0.0f, 0.f}, {1.0f, 1.0f, 0.0f}
    };
    const u32 idx[] = { 
        0, 3, 1, 0, 2, 3 
    };
    
    *vtx_count = sizeof(vtx) / sizeof(vtx[0]);
    *idx_count = sizeof(idx) / sizeof(idx[0]);
    
    if(vertices) {
        memcpy(vertices, vtx, sizeof(vtx));
    }
    
    if(indices) {
        memcpy(indices, idx, sizeof(idx));
    }
}

extern "C" __declspec(dllexport) GAME_LOOP(GameLoop) {
    
    const Uint8 *keyboard = SDL_GetKeyboardState(NULL);
    
    bool moved = false;
    
    vec3 movement = {};
    
    const float speed = delta_time;
    
    vec3 forward = { (float)sin(-game_data->rotation.y), 0.0f, (float)cos(-game_data->rotation.y)}; 
    vec3 right = { (float)sin(-game_data->rotation.y - PI/2.f), 0.0f, (float)cos(-game_data->rotation.y - PI/2.f)}; 
    
    if(keyboard[SDL_SCANCODE_W]) {
        movement = movement + vec3_fmul(forward, -speed);
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_S]) {
        
        movement = movement + vec3_fmul(forward, speed);
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_A]){
        movement = movement + vec3_fmul(right, -speed);
        //movement.x += speed;
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_D]){
        movement = movement + vec3_fmul(right, speed);
        //movement.x -= speed;
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_Q]){
        movement.y -= speed;
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_E]){
        movement.y += speed;
        moved = true;
    }
    
    // Reset
    if(keyboard[SDL_SCANCODE_SPACE]){
        GameStart(game_data);
    }
    
    i32 mouse_x;
    i32 mouse_y;
    u32 mouse_state = SDL_GetRelativeMouseState(&mouse_x, &mouse_y);
    
    if(mouse_state == SDL_BUTTON(3)) {
        if(mouse_x != 0) {
            game_data->rotation.y += speed * mouse_x * 2.5f;
            moved = true;
        }
        if(mouse_y != 0){
            float new_rot = game_data->rotation.x + speed * mouse_y * -2.5f;
            if(new_rot < PI/2.0f && new_rot > -PI/2.0f) {
                game_data->rotation.x = new_rot;
            }
            moved = true;
        }
        SDL_SetRelativeMouseMode(SDL_TRUE);
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
    
    if(moved) {
        mat4_rotate_euler(&game_data->matrices.view, {game_data->rotation.x, 0.0f, 0.0f});
        mat4_translate(&game_data->matrices.view, movement);
        
        
        game_data->position = game_data->position + movement;
        //game_data->matrices.view = mat4_look_at({0.0f,-0.5f,0.0f}, game_data->position, {0.0f,1.0f,0.0f} );
        //game_data->matrices.mesh = mat4_identity();
        moved = false;
    }
    
}