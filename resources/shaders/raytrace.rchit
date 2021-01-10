#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct HitPayload
{
  vec3 hit_value;
};

layout(location = 0) rayPayloadInEXT HitPayload prd;
hitAttributeEXT vec3 attribs;

layout(push_constant) uniform Constants
{
    vec4 clear_color;
    vec3 light_position;
    float light_instensity;
    int light_type;
};

void main()
{
  prd.hit_value = vec3(0.5, 0.5, 0.5);
}
