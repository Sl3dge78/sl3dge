#version 460

vec2 quad[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

layout(location = 0) out vec2 uv;

void main() {
    gl_Position = vec4(quad[gl_VertexIndex], 0.0, 1.0);
    uv = (quad[gl_VertexIndex] + vec2(1.0)) /2.0;

}