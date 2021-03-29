#include "game.h"

#include <string.h>

// Todo/idea list 
//
// Make our own console, in-engine
// 
//  

extern "C" __declspec(dllexport) GAME_START(GameStart) {
    game_data->cam_matrix = mat4_identity();
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
    
    cgltf_options options = {0};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, "resources\\triangle.gltf", &data);
    if(result == cgltf_result_success) {
        cgltf_load_buffers(&options, data,  "resources\\triangle.gltf");
        
        cgltf_free(data);
    }
    
    
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
    
    const float speed = 0.001f;
    
    if(keyboard[SDL_SCANCODE_W]) {
        movement.y -= speed;
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_S]) {
        movement.y += speed;
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_A]){
        movement.x -= speed;
        moved = true;
    }
    if(keyboard[SDL_SCANCODE_D]){
        movement.x += speed;
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
    if(keyboard[SDL_SCANCODE_SPACE]){
        GameStart(game_data);
    }
    
    if(moved) {
        mat4_translate(&game_data->cam_matrix, movement);
    }
    
}