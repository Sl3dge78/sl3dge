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

typedef struct GameData {
    // System stuff
    Console console;
    EventQueue event_queue;
    
    bool is_free_cam;
    Camera camera;
    
    Vec3 light_dir;
    f32 cos;
    
    MeshHandle floor;
    
    NPC npc;
    
    TransformHandle floor_xform;
    
    Vec3 interact_sphere_pos;
    f32 interact_sphere_diameter;
    
    MeshHandle simple_skinning;
    TransformHandle simple_skinning_root;
    
    bool show_anim_ui;
    bool show_shadowmap;
    
} GameData;

