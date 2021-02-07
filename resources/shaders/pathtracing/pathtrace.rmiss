#version 460
#extension  GL_GOOGLE_include_directive  : require
#extension GL_EXT_ray_tracing : require

#include "shader_utils.glsl"

layout(location = 0) rayPayloadInEXT HitPayloadSimple payload;

layout(push_constant) uniform Constants {
    vec4 clear_color;
};

void main() {
    payload.direct_color = clear_color.xyz * 0.8;
}