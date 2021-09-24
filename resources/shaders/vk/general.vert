#version 460

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;

layout (binding = 0) uniform CameraMatrices {
	mat4 proj;
    mat4 proj_inverse;
	mat4 view;
	mat4 view_inverse;
	mat4 light_vp;
	vec3 view_pos;
	vec3 light_dir;
} cam;

layout(location = 0) out vec3 worldpos;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 texcoord;
layout(location = 4) out uint material_id;
layout(location = 5) out vec4 shadow_map_texcoord;

layout(push_constant) uniform PushConstants {
	mat4 transform;
	uint material_id;
} constants;

const mat4 bias = mat4 (
0.5,0.0,0.0,0.0,
0.0,0.5,0.0,0.0,
0.0,0.0,1.0,0.0,
0.5,0.5,0.0,1.0 );

void main() {

	vec4 pos = constants.transform * vec4(in_position, 1.0);

	gl_Position = cam.proj * cam.view * pos;
    //gl_Position = cam.light_vp * pos;
	worldpos = pos.xyz;
	normal = normalize(transpose(inverse(mat3(constants.transform))) * in_normal);
	texcoord = in_texcoord;
	material_id = constants.material_id;
	shadow_map_texcoord = (cam.light_vp) * pos;
}