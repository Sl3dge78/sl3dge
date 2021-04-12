

    
    inline cgltf_buffer_view *GLTFGetIndicesBufferView(cgltf_data *data, u32 mesh_id) {
    
    return data->meshes[mesh_id].primitives[0].indices->buffer_view;
}

u32 GLTFGetPositionOffset(cgltf_data *data, u32 mesh_id) {
    
    cgltf_primitive *prim = &data->meshes[mesh_id].primitives[0];
    
    for(u32 i = 0; i < prim->attributes_count ; ++i) {
        
        if(prim->attributes[i].type == cgltf_attribute_type_position) {
            return prim->attributes[i].data->buffer_view->offset + prim->attributes[i].data->offset;
        }
    }
    SDL_LogError(0, "Unable to find position attribute");
    ASSERT(0);
    return 0;
}
