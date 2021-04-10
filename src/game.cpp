#include "game.h"

#include <string.h>

// Todo/idea list 
//
// Make our own console, in-engine
// 
//  

extern "C" __declspec(dllexport) GAME_START(GameStart) {
    game_data->cam_matrix = mat4_identity();
    game_data->rotation_z = 0.f;
    game_data->rotation_x = 0.f;
    mat4_translate(&game_data->cam_matrix, {0.5f, 1.0f, -0.1f});
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
    
    vec3 forward = { (float)sin(-game_data->rotation_z), (float)cos(-game_data->rotation_z), 0.f}; 
    vec3 right = { (float)sin(-game_data->rotation_z + PI/2.f), (float)cos(-game_data->rotation_z + PI/2.f), 0.f}; 
    
    if(keyboard[SDL_SCANCODE_W]) {
        vec3_add(&movement, vec3_fmul(forward, -speed));
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_S]) {
        vec3_add(&movement, vec3_fmul(forward, speed));
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_A]){
        
        vec3_add(&movement, vec3_fmul(right, -speed));
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_D]){
        vec3_add(&movement, vec3_fmul(right, speed));
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_Q]){
        movement.z += speed;
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_E]){
        movement.z -= speed;
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
            game_data->rotation_z += speed * mouse_x * 2.5f;
            moved = true;
        }
        if(mouse_y != 0){
            float new_rot = game_data->rotation_x + speed * mouse_y * -2.5f;
            SDL_Log("%f", new_rot);
            if(new_rot < PI/2.0f && new_rot > -PI/2.0f) {
                game_data->rotation_x = new_rot;
            }
            moved = true;
        }
        SDL_SetRelativeMouseMode(SDL_TRUE);
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
    
    if(moved) {
        mat4_rotate_euler(&game_data->cam_matrix, {game_data->rotation_x, 0, game_data->rotation_z});
        mat4_translate(&game_data->cam_matrix, movement);
    }
    
}