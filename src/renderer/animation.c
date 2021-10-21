internal void LoadAnimation(cgltf_animation *src, SkinnedMesh *mesh, Animation *result) {
    
    result->track_count = src->channels_count;
    result->tracks = sCalloc(result->track_count, sizeof(AnimationTrack));
    result->length = 0;
    
    sLog("LOAD - Animation - %d tracks", result->track_count);
    
    for(u32 i = 0; i < src->channels_count; i++) {
        AnimationTrack *track = &result->tracks[i];
        cgltf_animation_channel *channel = &src->channels[i];
        Transform *target = &mesh->joints[channel->target_node->id-1];
        switch(channel->target_path) {
            case cgltf_animation_path_type_translation: {
                track->target = &target->translation;
                track->type = ANIM_TYPE_VEC3;
            } break;
            case cgltf_animation_path_type_rotation: {
                track->target = &target->rotation; 
                track->type = ANIM_TYPE_QUATERNION;
            } break;
            case cgltf_animation_path_type_scale: {
                track->target = &target->scale; 
                track->type = ANIM_TYPE_VEC3;
            } break;
            default : {
                ASSERT_MSG(0, "Unhandled channel type");
            } break;
        }
        cgltf_accessor *input = channel->sampler->input;
        track->key_count = input->count;
        track->key_times = sCalloc(track->key_count, sizeof(f32));
        GLTFCopyAccessor(input, track->key_times, 0, (u32)input->stride);
        
        cgltf_accessor *output = channel->sampler->output;
        track->keys = sCalloc(output->count, output->stride);
        GLTFCopyAccessor(output, track->keys, 0, (u32)output->stride);
        ASSERT_MSG(output->count == input->count, "gltf has a different amount of key times and data keys. Blender has a bug that does this apparently...");
        
        if (result->length < track->key_times[track->key_count - 1]) 
            result->length = track->key_times[track->key_count - 1];
        
    }
}

void DestroyAnimation(Animation *anim) {
    for(u32 i = 0; i < anim->track_count; i++) {
        sFree(anim->tracks[i].keys);
        sFree(anim->tracks[i].key_times);
    }
    sFree(anim->tracks);
}

void AnimationEvaluate(Animation *a, f32 time) {
    
    // clamp time
    if (time > a->length)
        time = a->length;
    
    for(u32 i = 0; i < a->track_count; i++) {
        AnimationTrack *track = &a->tracks[i];
        
        u32 key_1 = 0; u32 key_2 = 0;
        if(time > track->key_times[track->key_count - 1]) {
            key_1 = key_2 = track->key_count - 1;
        }
        else {
            for(u32 i = 0; i < track->key_count; i ++) {
                if(time > track->key_times[i] && time < track->key_times[i + 1]) {
                    key_1 = i;
                    key_2 = i + 1;
                    
                    break;
                }
            }
        }
        
        f32 time_between_keys = track->key_times[key_2] - track->key_times[key_1];
        f32 rel_t = time - track->key_times[key_1];
        f32 norm_t = rel_t / time_between_keys;
        
        switch(track->type) {
            case ANIM_TYPE_QUATERNION : {
                Quat *keys = (Quat *)track->keys;
                if(key_1 == key_2) {
                    *(Quat*)track->target = keys[key_2];
                } else {
                    *(Quat*)track->target = quat_slerp(keys[key_1], keys[key_2], norm_t);
                }
            } break;
            case ANIM_TYPE_VEC3 : {
                Vec3 *keys = (Vec3 *)track->keys;
                if(key_1 == key_2) {
                    *(Vec3 *)track->target = keys[key_2];
                } else {
                    *(Vec3 *)track->target = vec3_lerp(keys[key_1], keys[key_2], norm_t);
                }
            } break;
            /* TODO(Guigui): 
            case ANIM_TYPE_FLOAT : {
                } break;
            case ANIM_TYPE_TRANSFORM : {
                } break;
    */
            default : ASSERT(0); break;
        }
    }
}