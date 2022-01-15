#pragma once

typedef struct Camera {
    Vec3 position;
    Vec2f spherical_coordinates;
    Vec3 forward;
} Camera;

typedef struct NPC {
    
    TransformHandle xform;
    MeshHandle mesh;
    SkinHandle skin;
    AnimationHandle walk_animation;
    
    f32 anim_time;
    
    Vec3 destination;
    f32 walk_speed;
    f32 distance_to_dest;
    
} NPC;

typedef enum Direction {
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
} Direction;

typedef struct GameData {
    // System stuff
    Console console;
    EventQueue event_queue;
    
    Camera camera;
    bool is_free_cam;
    
    Vec3 light_dir;
    f32 cos;
    
    TransformHandle floor_xform;
    MeshHandle floor;
    
    NPC npc;
    
    MeshHandle cube;

    TransformHandle char_xform;
    MeshHandle sword;
    TransformHandle sword_xform;

    Direction player_direction;
    f32 attack_time;

    Quat sword_start_rot;
    Quat sword_end_rot;
    Vec3 sword_offset;
    
    bool enemy_collided;
    u32 enemy_health;
    TransformHandle enemy_xform;

    MeshHandle simple_skinning;
    TransformHandle simple_skinning_root;
    
    bool show_anim_ui;
    bool show_shadowmap;
    
} GameData;

