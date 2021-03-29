#include "game.h"

#include <string.h>

// Todo/idea list 
//
// Make our own console, in-engine
// 
//  

extern "C" __declspec(dllexport) GAME_GET_SCENE(GameGetScene) {
    
    const vec3 vtx[] = {
        {-0.5f, -1.f, -0.5f}, {0.5f, -1.f, -0.5f},
        {-0.5f, -2.0f, -0.5f}, {0.5f, -2.0f, -0.5f}
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
    SDL_Log("Hoy");
    
}