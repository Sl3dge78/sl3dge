#version 460

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (binding = 0) uniform CameraMatrices {
 mat4 proj;
 mat4 view;
 mat4 mesh;
} cam;

layout(location = 0) out vec3 normal;

void main() {
	
	gl_Position = cam.proj * cam.view * vec4(in_position, 1.0);
	normal = in_normal;
    
}