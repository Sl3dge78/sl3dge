#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D tex_sampler;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 tex_coords;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(tex_sampler, tex_coords);
   //out_color = vec4(tex_coords, 0.0, 1.0);
}