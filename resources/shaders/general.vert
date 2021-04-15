#version 460

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;

layout (binding = 0) uniform CameraMatrices {
	mat4 proj;
	mat4 view;
	vec3 pos;
	vec3 view_dir;
} cam;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec3 frag_pos;
layout(location = 2) out vec3 cam_pos;
layout(location = 3) out uint material_id;

layout(push_constant) uniform PushConstants {
	mat4 transform;
	uint material_id;
} constants;

void main() {
	
	vec4 pos = cam.proj * cam.view * constants.transform * vec4(in_position, 1.0);
	
	gl_Position = pos;
	normal = in_normal;
	cam_pos = cam.pos;
	material_id = constants.material_id;
	frag_pos = pos.xyz;
}