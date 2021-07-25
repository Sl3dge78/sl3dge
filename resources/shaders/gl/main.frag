#version 330 core

in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D diffuse;

void main() {
    //FragColor = vec4(TexCoord, 0.0, 1.0);
    FragColor = texture(diffuse, TexCoord);
}