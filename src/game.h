#pragma once

typedef struct Camera {
    Vec3 position;
    Vec2f spherical_coordinates;
    Vec3 forward;
} Camera;

typedef struct NPC {
    EntityID entity;
    Animation walk_animation;

    f32 anim_time;
    
    Vec3 destination;
    f32 walk_speed;
    f32 distance_to_dest;
} NPC;

typedef struct GameData {
    // System stuff
    Console console;
    EventQueue event_queue;
    World world;
    
    Camera camera;
    bool is_free_cam;
    
    Vec3 light_dir;
    f32 cos;
    
    MeshHandle mesh_quad;
    MeshHandle mesh_cube;

    EntityID ground;
    EntityID player;
    EntityID sword;

    u32 enemy_count;
    EntityID *enemies;

    NPC npc;
    
    f32 attack_time;

    Quat sword_start_rot;
    Quat sword_end_rot;
    Vec3 sword_offset;
    
    bool show_anim_ui;
    bool show_shadowmap;
    
} GameData;

