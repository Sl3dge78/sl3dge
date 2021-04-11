#version 460

// layout (location = 0) in vec3 in_position;

vec3 positions [3] =  vec3[] (
vec3(0.0, 0.0, 0.0),
vec3(1.0, 0.0, 0.0),
vec3(-1.0, 0.0, 0.0)
);

void main() {

	 gl_Position = vec4(positions[gl_VertexIndex], 1.0);
   // gl_Position = vec4(in_position, 1.0);
}