#include "game.h"

// Todo/idea list 
//
// Make our own console, in-engine
// 
//  

extern "C" __declspec(dllexport) GAME_LOOP(GameLoop) {
    SDL_Log("Hoy");
    
}

// Set the binding_count & push_constant_count values with the amount required.
// Fills bindings & push_constants values only if they are not NULL. 
extern "C" __declspec(dllexport) GAME_GET_DESCRIPTORS_INFO(GameGetDescriptorsInfo){
    
    u32 mesh_count = 1;// TODO(Guigui): 
    u32 material_count = 1;// TODO(Guigui): 
    
    *count = 5;
    if(bindings) {
        bindings[0] = { // CAMERA MATRICES
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            NULL
        };
        
        bindings[1] = { // SCENE DESCRIPTION
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            NULL
        };
        
        // TODO(Guigui): Merge these two buffers into one and provide an offset
        bindings[2] = { // VERTEX BUFFER
            2,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            mesh_count,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            NULL
        };
        
        bindings[3] = { // INDEX BUFFER
            3,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            mesh_count,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            NULL
        };
        
        bindings[4] = { // MATERIAL BUFFER
            4,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            material_count,
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            NULL
        };
    }
}

extern "C" __declspec(dllexport) GAME_GET_DESCRIPTORS_WRITES(GameGetDescriptorWrites) {
    
    /*
    *count = 1;
    
    if(writes) {
        
        /*
                // CAMERA MATRICES
                
                
                
                // SCENE DESCRIPTION
                writes[2] = {
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    NULL,
                    descriptor_set,2,
                };
                
                // VERTEX BUFFER
                writes[3] = {
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    NULL,
                    descriptor_set,3,
                };
                
                // INDEX BUFFER
                writes[4] = {
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    NULL,
                    descriptor_set,4,
                };
                // MATERIAL BUFFER
                writes[5] = {
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    NULL,
                    descriptor_set,5,
                };
                */
    
}

extern "C" __declspec(dllexport) GAME_GET_POOL_SIZES(GameGetPoolSizes) {
    
    *count = 2;
    
    if(pool_sizes) {
        pool_sizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
        pool_sizes[1] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4 };
    }
}

extern "C" __declspec(dllexport) GAME_GET_PUSH_CONSTANTS(GameGetPushConstants) {
    
    if(push_constants) {
        push_constants[0] = { VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 0, sizeof(PushConstant) };
    }
}
