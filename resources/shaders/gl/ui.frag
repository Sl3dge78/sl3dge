#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D alpha_map; // TEXTURE0
uniform sampler2D color_texture; // TEXTURE1
uniform vec4 color;

void main() {
    vec4 tex = texture(color_texture, TexCoord);
    float alpha = texture(alpha_map, TexCoord).r;
    //FragColor = texture(glyphs, TexCoord);
    FragColor = vec4(color.r * tex.r, color.g * tex.g, color.b * tex.b, color.a * alpha);
}
