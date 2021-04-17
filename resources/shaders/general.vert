#version 460

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;

layout (binding = 0) uniform CameraMatrices {
	mat4 proj;
	mat4 view;
	vec3 pos;
	vec3 view_dir;
} cam;

layout(location = 0) out vec3 worldpos;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 texcoord;
layout(location = 3) out vec3 cam_pos;
layout(location = 4) out uint material_id;

layout(push_constant) uniform PushConstants {
	mat4 transform;
	uint material_id;
} constants;

void main() {
	
	vec4 pos = constants.transform * vec4(in_position, 1.0);
	
	gl_Position = cam.proj * cam.view * pos;
	worldpos = pos.xyz;
	normal = normalize(transpose(inverse(mat3(constants.transform))) * in_normal);
	texcoord = in_texcoord;
	cam_pos = cam.pos;
	material_id = constants.material_id;
}