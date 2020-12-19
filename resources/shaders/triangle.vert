#version 450


layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;


layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coords;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 tex_coords;

void main() {
    gl_Position = ubo.proj *  ubo.view * ubo.model * vec4(in_position, 1.0);
    normal = in_normal;
    tex_coords = in_tex_coords;
    
}