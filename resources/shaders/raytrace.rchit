#version 460
#extension  GL_GOOGLE_include_directive  : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "shader_utils.h"

layout(binding = 0, set = 0) uniform accelerationStructureEXT top_level_AS;
layout(binding = 3, set = 0) buffer Scene {Instance i[];} scene;
layout(binding = 4, set = 0) buffer Vertices {Vertex v[];} vertices[];
layout(binding = 5, set = 0) buffer Indices {uint i[];} indices[];

layout(push_constant) uniform Constants
{
    vec4 clear_color;
    vec3 light_position;
    float light_instensity;
    int light_type;
} constants;

layout(location = 0) rayPayloadInEXT HitPayload prd;
layout(location = 1) rayPayloadInEXT bool is_shadow;
hitAttributeEXT vec3 attribs;

void main() {

    uint mesh_id = scene.i[gl_InstanceCustomIndexEXT].mesh_id;
    ivec3 idx = ivec3(indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 0], 
                    indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 1], 
                    indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 2]);

    Vertex v0 = vertices[nonuniformEXT(mesh_id)].v[idx.x];
    Vertex v1 = vertices[nonuniformEXT(mesh_id)].v[idx.y];
    Vertex v2 = vertices[nonuniformEXT(mesh_id)].v[idx.z];

    const vec3 barycentre = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 normal = normalize(v0.normal * barycentre.x + v1.normal * barycentre.y + v2.normal * barycentre.z);
    normal = normalize(vec3(scene.i[gl_InstanceCustomIndexEXT].inverted * vec4(normal, 0.0)));

    vec3 world_pos = v0.pos * barycentre.x + v1.pos * barycentre.y + v2.pos * barycentre.z;
    world_pos = vec3(scene.i[gl_InstanceCustomIndexEXT].transform * vec4(world_pos, 1.0));

    const vec3 light_dir = normalize(constants.light_position - world_pos);
    const float dnl = dot(normal, light_dir);

    float light_distance = 100000.0; // Compute for point lights

    float ambient = 0.1;
    float diffuse = max(dnl, 0.0);
    float attenuation = 1;

    if(dnl>=0) {
        // Throw a ray in the light's direction. If we hit nothing (miss shader) we're not in shadow.
        float t_min = 0.0;
        float t_max = light_distance;
        vec3 origin = world_pos + normal * 0.003f;
        vec3 dir = light_dir;
        uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

        is_shadow = true;
        traceRayEXT(top_level_AS,   // Structure
                flags,          // Flags
                0xff,           // Mask
                0,              // sbt record offset
                0,              // sbt record stride
                1,              // miss shader index
                origin,         
                t_min,
                dir,
                t_max, 
                1               // Payload location
            );
  

 
        if(is_shadow) {
            attenuation = 0.3;
        } else {
            attenuation = 1;
        }
    }
  
    vec3 color = vec3(1.0,1.0,1.0);
    float lighting = max(diffuse, ambient);
    prd.hit_value = color * (diffuse + ambient) * attenuation;
}
