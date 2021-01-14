#version 460
#extension  GL_GOOGLE_include_directive  : require
#extension GL_EXT_ray_tracing : require

#include "shader_utils.h"

layout(location = 0) rayPayloadInEXT HitPayload prd;

layout(push_constant) uniform Constants
{
    vec4 clear_color;
};

void main()
{
    prd.hit_value = clear_color.xyz * 0.8;
}