// I just put random stuff here that isn't used any more but that i want to keep

void DrawAnimationUI(PushBuffer *ui, Animation *animation, f32 time, u32 bone, u32 key) {
    u32 x = 10;
    u32 y = global_renderer->height - 80;
    u32 w = global_renderer->width - 40;
    u32 h = 70;
    
    // Panel
    PushUIQuad(ui, x, y, w, h, (Vec4){0.1f, 0.1f, 0.1f, 0.5f});
    
    // Text
    PushUIFmt(ui, x + 5, y + 20, (Vec4){1.0f, 1.0f, 1.0f, 1.0f}, "Current Time: %.2f, Key: %d, Bone: %d", time, key, bone);
    
    // Timeline
    PushUIQuad(ui, x + 5, y + 50, w - 10, 10, (Vec4){0.1f, 0.1f, 0.1f, 0.8f});
    
    AnimationTrack *track = &animation->tracks[0];
    
    // Keys
    for(u32 i = 0; i < track->key_count; i++) {
        f32 rel_t = track->key_times[i] / animation->length;
        Vec4 color = (Vec4){1.0f, 0.5f, 0.5f, 1.0f};
        if(key == i)
            color.y = 0.0f;
        PushUIQuad(ui, x + 5 + rel_t * (w - 10), y + 50, 1, 10, color);
    }
    
    // Playhead
    f32 rel_time = time / animation->length;
    PushUIQuad(ui, x + 5 + rel_time * w, y + 45, 1, 20, (Vec4){0.9f, 0.9f, 0.9f, 1.0f});
    
    
    Quat v1;
    Quat v2;
    u32 track_id = 0;
    for(u32 i = 0; i < animation->track_count; i ++){
        AnimationTrack *t = &animation->tracks[i];
        if(t->target == ANIM_TARGET_ROTATION && t->target_node == bone) {
            track_id = i;
            Quat *keys = (Quat *)(t->keys);
            v1 = keys[key];
            v2 = keys[key + 1];
            
            /*
            f32 time_between_keys = t->key_times[key + 1] - t->key_times[key];
            f32 rel_t = time - track->key_times[key];
            f32 norm_t = rel_t / time_between_keys;
            
            Quat q_lerp = quat_slerp(keys[key], keys[key + 1], norm_t);
            quat_sprint(q_lerp, qlerp);
            */
            break;
        }
    }
    
}
