#version 460
#extension  GL_GOOGLE_include_directive  : require
#extension GL_EXT_ray_tracing : require

#include "shader_utils.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

layout(push_constant) uniform Constants {
    vec4 clear_color;
};

void main() {
    if(payload.depth == 0) {
        payload.direct_color = clear_color.xyz * 0.8;
    } else {
        payload.direct_color = vec3(1);
    }
    payload.depth = 100;   
}