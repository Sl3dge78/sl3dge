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
    MeshHandle character;
    
    Transform *floor_xform;
    Transform *npc_xform;
    
    Vec3 interact_sphere_pos;
    f32 interact_sphere_diameter;
    
    Mesh *simple_skinning;
    Transform *simple_skinning_root;
    
    Transform *cylinder_xform;
    Mesh *cylinder_mesh;
    Skin *cylinder_skin;
    
    f32 anim_time;
    Animation anim;
    Animation anim_t;
    
    bool show_shadowmap;
    
} GameData;

