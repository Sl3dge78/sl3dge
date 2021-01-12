#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct HitPayload
{
  vec3 hit_value;
};

struct Vertex {
  vec3 pos;
	vec3 normal;
	vec2 tex_coord;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT top_level_AS;
layout(binding = 3, set = 0) buffer Vertices {Vertex v[];} vertices[];
layout(binding = 4, set = 0) buffer Indices {uint i[];} indices[];

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
