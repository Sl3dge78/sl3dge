#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 Normal;
out vec2 TexCoord;
out vec4 shadow_map_texcoord;
out vec3 worldpos;

uniform mat4 transform;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 light_matrix;

void main() {
    vec4 pos = vec4(aPos, 1.0);
    worldpos = pos.xyz;
    Normal = aNormal;
    TexCoord = aTexCoord;
    shadow_map_texcoord = light_matrix * pos;
    gl_Position =  projection * view * pos;
}