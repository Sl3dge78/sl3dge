#version 460

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;

layout (binding = 0) uniform CameraMatrices {
 mat4 proj;
 mat4 view;
 mat4 mesh;
 vec3 pos;
} cam;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec3 frag_pos;
layout(location = 2) out vec3 cam_pos;

void main() {
	
	vec3 position = in_position;
	position.x += gl_InstanceIndex * 1.5;

	gl_Position = cam.proj * cam.view * vec4(position, 1.0);
	normal = in_normal;
	cam_pos = cam.pos;
}