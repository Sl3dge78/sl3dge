#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D screen_texture;

void main () {

    vec4 color = texture(screen_texture, TexCoord);
    //FragColor = vec4(color.r, color.r, color.r, 1.0);
    FragColor = color;


}