#version 460

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;

layout (binding = 0) uniform CameraMatrices {
	mat4 proj;
	mat4 view;
    mat4 shadow_mvp;
	vec3 pos;
	vec3 view_dir;
} cam;

layout(push_constant) uniform PushConstants {
	mat4 transform;
	uint material_id;
} constants;

void main() {

    gl_Position = cam.proj * cam.shadow_mvp * constants.transform * vec4(in_position, 1.0);

}