#version 460
#extension GL_EXT_nonuniform_qualifier : enable

struct Material {
	float ambient_intensity;
	vec3 diffuse_color;
	int texture_id;
};
struct Instance {
	uint mesh_id;
	uint mat_id;
	mat4 transform;
	mat4 inverted;
};

layout(set = 0, binding = 0) uniform CameraMatrices {
    mat4 view;
    mat4 proj;
    mat4 view_inverse;
    mat4 proj_inverse;
    int frame;
} cam;
layout (set = 0, binding = 3) buffer Scene {Instance i[];} scene;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coords;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 tex_coords;
layout(location = 2) out vec3 frag_pos;
layout(location = 3) out uint mat_id;

void main() {
    Instance instance = scene.i[gl_InstanceIndex];

    vec4 pos = instance.transform * vec4(in_position, 1.0);
    gl_Position = cam.proj * cam.view * pos;
    
    //view_pos =  (cam.view * vec4(1.0)).xyz;

    mat_id = instance.mat_id;
    frag_pos = pos.xyz;
    normal = in_normal;
    tex_coords = in_tex_coords;
}
