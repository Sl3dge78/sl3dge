#pragma once

typedef struct Camera {
    Vec3 position;
    Vec2f spherical_coordinates;
    Vec3 forward;
} Camera;

typedef struct GameData {
    // System stuff
    Console console;
    EventQueue event_queue;
    
    bool is_free_cam;
    Camera camera;
    
    Vec3 light_dir;
    f32 cos;
    
    MeshHandle floor;
    
    TransformHandle character_xform;
    MeshHandle character;
    SkinHandle character_skin;
    AnimationHandle walk;
    
    TransformHandle floor_xform;
    
    Vec3 interact_sphere_pos;
    f32 interact_sphere_diameter;
    
    MeshHandle simple_skinning;
    TransformHandle simple_skinning_root;
    
    TransformHandle cylinder_xform;
    MeshHandle cylinder_mesh;
    SkinHandle cylinder_skin;
    
    TransformHandle cube_xform;
    MeshHandle cube;
    
    f32 anim_time;
    
    bool show_anim_ui;
    bool show_shadowmap;
    
} GameData;

