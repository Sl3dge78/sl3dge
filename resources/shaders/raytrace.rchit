#version 460
#extension GL_EXT_ray_tracing : require

struct Payload {
    vec3 direct_color;
};

struct Vertex {
	vec3 pos;
	vec3 normal;
	vec2 tex_coord;
};

struct Instance {
	uint mesh_id;
	uint mat_id;
	mat4 transform;
	mat4 inverted;
};

struct Material {
	vec3 albedo;
    int albedo_texture_id;
    float metallic;
    float roughness;
    float ao;
    float rim_pow;
    float rim_strength;
};

layout(location = 0) rayPayloadInEXT Payload payload;

layout(binding = 1, set = 0) uniform accelerationStructureEXT tlas;
layout(binding = 1, set = 1) uniform Cam {
	mat4 view;
	mat4 proj;
} cam;

/*
layout(binding = 2, set = 0) uniform CameraMatrices {
    mat4 view;
    mat4 proj;
    mat4 view_inverse;
    mat4 proj_inverse;
    vec3 pos;
    int frame;
} cam;
layout(binding = 3, set = 0) buffer Scene {Instance i[];} scene;
layout(binding = 4, set = 0) buffer Vertices {Vertex v[];} vertices[];
layout(binding = 5, set = 0) buffer Indices {uint i[];} indices[];
layout(binding = 6, set = 0) buffer Materials {Material m[];} materials;
*/

void main() {

    payload.direct_color = vec3(1.0,1.0,0.0);

}