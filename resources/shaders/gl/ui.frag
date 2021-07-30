#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D glyphs;
uniform vec4 color;

void main() {
    float alpha = texture(glyphs, TexCoord).r;
    FragColor = vec4(color.r, color.g, color.b, color.a * alpha);
}