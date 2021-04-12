#version 460

layout (location = 0) in vec3 in_position;
layout (binding = 0) uniform CameraMatrices {
 mat4 proj;
 mat4 view;
 mat4 mesh;
} cam;

void main() {
	
	gl_Position = cam.proj * cam.view * vec4(in_position, 1.0);
    //gl_Position = cam.mesh * vec4(in_position, 1.0);

}