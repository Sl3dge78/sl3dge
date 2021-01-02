#version 450
#extension GL_ARB_separate_shader_objects : enable

struct SunLight {
    vec3 light_direction;
    vec3 light_color;
    float strength;
};

struct PointLight {
	vec3 position;
	vec3 color;
	float strength;
};

layout(binding = 1) uniform sampler2D tex_sampler;
layout(binding = 2) uniform FragUniformBufferObject {
    vec3 view_position;

    SunLight sun_light;

    float ambient_strength;
    float diffuse_strength;
    float specular_strength;
    float shininess;
} fubo;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in vec3 frag_pos;

layout(location = 0) out vec4 out_color;


void main() {
    SunLight sun = fubo.sun_light;

    vec3 ambient = fubo.ambient_strength * sun.light_color;

    float diff = max(dot(normal, -sun.light_direction), 0.0);
    vec3 diffuse = fubo.diffuse_strength * diff * sun.light_color;

    vec3 view_dir = normalize(fubo.view_position - frag_pos);
    vec3 reflect_dir = reflect(sun.light_direction, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), fubo.shininess);
    vec3 specular = fubo.specular_strength * spec * sun.light_color;

    out_color = vec4(ambient + diffuse + specular, 1.0) * texture(tex_sampler, tex_coords);
}
