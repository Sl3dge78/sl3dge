#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D tex_sampler;
layout(binding = 2) uniform FragUniformBufferObject {
     vec3 view_position;
    vec3 light_position;
    vec3 light_color;
} fubo;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in vec3 frag_pos;

layout(location = 0) out vec4 out_color;


void main() {
    float ambient_strength = 0.01;
    vec3 ambient = ambient_strength * fubo.light_color;

    vec3 light_dir = normalize(fubo.light_position - frag_pos);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diff * fubo.light_color;

    out_color = vec4(ambient + diffuse, 1.0) * texture(tex_sampler, tex_coords);
   //out_color = vec4(tex_coords, 0.0, 1.0);
}