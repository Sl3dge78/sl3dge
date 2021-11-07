
void LoadAnimation(Animation *result, const GLTF *gltf) {
    
    ASSERT_MSG(gltf->animation_count == 1, "ASSERT : More than one animation in GLTF, this isn't handled yet.");
    
    GLTFAnimation *anim = &gltf->animations[0];
    
    result->track_count = anim->channel_count;
    result->tracks = sCalloc(result->track_count, sizeof(AnimationTrack));
    result->length = 0;
    
    sLog("LOAD - Animation - %d tracks", result->track_count);
    
    for(u32 i = 0; i < anim->channel_count; i++) {
        AnimationTrack *track = &result->tracks[i];
        GLTFChannel *channel = &anim->channels[i];
        track->target_node = GLTFGetBoneIDFromNode(gltf, channel->target_node);
        
        u32 key_size;
        
        switch(channel->path) {
            case ANIMATION_PATH_TRANSLATION: {
                track->target = ANIM_TARGET_TRANSLATION;
                track->type = ANIM_TYPE_VEC3;
                key_size = sizeof(Vec3);
            } break;
            case ANIMATION_PATH_ROTATION: {
                track->target = ANIM_TARGET_ROTATION; 
                track->type = ANIM_TYPE_QUATERNION;
                key_size = sizeof(Quat);
            } break;
            case ANIMATION_PATH_SCALE: {
                track->target = ANIM_TARGET_SCALE; 
                track->type = ANIM_TYPE_VEC3;
                key_size = sizeof(Vec3);
            } break;
            default : {
                ASSERT_MSG(0, "Unhandled channel type");
            } break;
        }
        
        GLTFSampler *sampler = &anim->samplers[channel->sampler];
        GLTFAccessor *input = &gltf->accessors[sampler->input];
        
        track->key_count = input->count;
        track->key_times = sCalloc(track->key_count, sizeof(f32));
        GLTFCopyAccessor(gltf, sampler->input, track->key_times, 0, sizeof(f32));
        
        GLTFAccessor *output_acc = &gltf->accessors[sampler->output];
        
        track->keys = sCalloc(output_acc->count, key_size);
        GLTFCopyAccessor(gltf, sampler->output, track->keys, 0, key_size);
        ASSERT_MSG(output_acc->count == input->count, "Gltf has a different amount of key times and data keys. Blender has a bug that does this apparently...");
        
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

void AnimationEvaluate(Renderer *renderer, TransformHandle first_joint, const u32 count, const AnimationHandle animation, f32 time) {
    
    Animation *a = ArrayGetElementAt(renderer->animations, animation);
    
    // clamp time
    if (time > a->length)
        time = a->length;
    if (time < 0)
        time = 0;
    
    for(u32 i = 0; i < a->track_count; i++) {
        AnimationTrack *track = &a->tracks[i];
        
        u32 key_1 = 0; u32 key_2 = 0;
        if(time >= track->key_times[track->key_count - 1]) {
            key_1 = key_2 = track->key_count - 1;
        }
        else {
            for(u32 i = 0; i < track->key_count; i ++) {
                if(time >= track->key_times[i] && time < track->key_times[i + 1]) {
                    key_1 = i;
                    key_2 = i + 1;
                    
                    break;
                }
            }
        }
        
        f32 time_between_keys = track->key_times[key_2] - track->key_times[key_1];
        f32 rel_t = time - track->key_times[key_1];
        f32 norm_t = rel_t / time_between_keys;
        
        if(track->target_node >= count)
            continue;
        
        Transform *target_xform = ArrayGetElementAt(renderer->transforms, first_joint + track->target_node);
        
        switch(track->target) {
            case ANIM_TARGET_TRANSLATION : {
                ASSERT(track->type == ANIM_TYPE_VEC3);
                Vec3 *keys = (Vec3 *)track->keys;
                if(key_1 == key_2) {
                    target_xform->translation = keys[key_2];
                } else {
                    target_xform->translation = vec3_lerp(keys[key_1], keys[key_2], norm_t);
                }
            } break;
            case ANIM_TARGET_ROTATION : {
                ASSERT(track->type == ANIM_TYPE_QUATERNION);
                Quat *keys = (Quat *)track->keys;
                if(key_1 == key_2) {
                    target_xform->rotation = keys[key_2];
                } else {
                    target_xform->rotation = quat_slerp(keys[key_1], keys[key_2], norm_t);
                }
            } break;
            case ANIM_TARGET_SCALE : {
                ASSERT(track->type == ANIM_TYPE_VEC3);
                Vec3 *keys = (Vec3 *)track->keys;
                if(key_1 == key_2) {
                    target_xform->scale = keys[key_2];
                } else {
                    target_xform->scale  = vec3_lerp(keys[key_1], keys[key_2], norm_t);
                }
            } break;
        }
    }
}