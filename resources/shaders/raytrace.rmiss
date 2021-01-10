#version 460
#extension GL_EXT_ray_tracing : require

struct HitPayload
{
  vec3 hit_value;
};

layout(location = 0) rayPayloadInEXT HitPayload prd;

layout(push_constant) uniform Constants
{
    vec4 clear_color;
};

void main()
{
    prd.hit_value = clear_color.xyz * 0.8;
}