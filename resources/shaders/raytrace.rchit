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

struct Instance {
  uint mesh_id;
  mat4 transform;
  mat4 inverted;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT top_level_AS;
layout(binding = 3, set = 0) buffer Scene {Instance i[];} scene;
layout(binding = 4, set = 0) buffer Vertices {Vertex v[];} vertices[];
layout(binding = 5, set = 0) buffer Indices {uint i[];} indices[];

layout(location = 0) rayPayloadInEXT HitPayload prd;
hitAttributeEXT vec3 attribs;

layout(push_constant) uniform Constants
{
    vec4 clear_color;
    vec3 light_position;
    float light_instensity;
    int light_type;
} constants;

vec3 BaryLerp(vec3 a, vec3 b, vec3 c, vec3 barycentre) {
    return a * barycentre.x + b * barycentre.y + c * barycentre.z;
}

void main()
{

  uint mesh_id = scene.i[gl_InstanceCustomIndexEXT].mesh_id;
  ivec3 idx = ivec3(indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 0], 
                    indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 1], 
                    indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 2]);

  Vertex v0 = vertices[nonuniformEXT(mesh_id)].v[idx.x];
  Vertex v1 = vertices[nonuniformEXT(mesh_id)].v[idx.y];
  Vertex v2 = vertices[nonuniformEXT(mesh_id)].v[idx.z];

  const vec3 barycentre = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  vec3 normal = normalize(v0.normal * barycentre.x + v1.normal * barycentre.y + v2.normal * barycentre.z);
  //normal = normalize(vec3(scene.i[gl_InstanceCustomIndexEXT].inverted * vec4(normal, 0.0)));

  vec3 world_pos = v0.pos * barycentre.x + v1.pos * barycentre.y + v2.pos * barycentre.z;
  //world_pos = vec3(scene.i[gl_InstanceCustomIndexEXT].transform * vec4(world_pos, 1.0));

  const float ambient = .2;

  vec3 light_dir = normalize(vec3(1,1,1) - world_pos);
  float diffuse = max(dot(normal, light_dir), 0.0);
  prd.hit_value = vec3(1.0,1.0,1.0) * (ambient + diffuse);
}
