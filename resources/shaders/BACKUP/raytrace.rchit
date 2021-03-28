#version 460
#extension GL_GOOGLE_include_directive  : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "shader_utils.glsl"
#include "random.glsl"

layout(binding = 1, set = 0) uniform accelerationStructureEXT top_level_AS;
layout(binding = 2, set = 0) uniform CameraMatrices {
    mat4 view;
    mat4 proj;
    mat4 view_inverse;
    mat4 proj_inverse;
    vec3 pos;
    int frame;
} cam;
layout(binding = 3, set = 0) buffer Scene {Instance i[];} scene;
layout(binding = 4, set = 0) buffer Vertices {Vertex v[];} vertices[];
layout(binding = 5, set = 0) buffer Indices {uint i[];} indices[];
layout(binding = 6, set = 0) buffer Materials {Material m[];} materials;

layout(push_constant) uniform Constants {
    vec4 clear_color;
    vec3 light_dir;
    float light_intensity;
    vec3 light_color;
} constants;

layout(location = 0) rayPayloadInEXT HitPayloadSimple payload;
layout(location = 1) rayPayloadInEXT bool is_shadow;
hitAttributeEXT vec3 attribs;

float geometry_schlick_ggx(float NdotV, float roughness){
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
} 
float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness){
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = geometry_schlick_ggx(NdotV, roughness);
    float ggx2 = geometry_schlick_ggx(NdotL, roughness);
	
    return ggx1 * ggx2;
}
float distribution_ggx(vec3 N, vec3 H, float roughness) {

    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N,H),0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = M_PI * denom * denom;
    
    return nom / denom;
}
vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cos_theta, 0.0), 5.0);
}
float rim(vec3 N, vec3 V, float power, float strength) {

    float rim = 1.0 - clamp(dot(N, V), 0.0, 1.0);
    rim = clamp(pow(rim, power) * strength, 0.0, 1.0);
    return rim;

}
vec3 pbr(Material mat, vec2 tex_coord, vec3 V, vec3 N) {

    vec3 albedo = mat.albedo;
    /*
     if(mat.albedo_texture_id >= 0) {
         albedo *= texture(textures[mat.albedo_texture_id], tex_coord).xyz;
    }
    */
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, mat.albedo, mat.metallic);

    vec3 Lo = vec3(0.0);

    //for (int i = 0; i < 1 ; i++) {
        //Light light = lights.l[0];
        /*if (light.type == 1) {
            L = normalize(light.vec - frag_pos);  
            float dist = length(light.vec - frag_pos);
            float attenuation = 1.0 / (dist * dist);
            radiance = light.color * light.intensity * attenuation;
        } else if (light.type == 0) {*/
            vec3 L = constants.light_dir;
            vec3 radiance = constants.light_color * constants.light_intensity;
        //}
        vec3 H = normalize(V + L);
        vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);
        float NDF = distribution_ggx(N, H, mat.roughness);
        float G = geometry_smith(N, V, L, mat.roughness);

         float NdotL = max(dot(N, L), 0.0);

        vec3 numer = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * NdotL;
        vec3 specular = numer / max(denom, 0.001);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - mat.metallic;

        vec3 rim_light = vec3(1.0) * rim(N, V, mat.rim_pow, mat.rim_strength);

        // Main color
        Lo += (kD * albedo / M_PI + specular + rim_light) * radiance * NdotL;
    // }
    
    vec3 ambient = vec3(0.03) * albedo * mat.ao;
    //ambient = vec3(0.0);
    vec3 color = Lo + ambient;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    return color;
}


void main() {

    uint mesh_id = scene.i[gl_InstanceCustomIndexEXT].mesh_id;
    uint mat_id = scene.i[gl_InstanceCustomIndexEXT].mat_id;
    Material mat = materials.m[mat_id];

    ivec3 idx = ivec3(indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 0], 
                    indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 1], 
                    indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 2]);

    Vertex v0 = vertices[nonuniformEXT(mesh_id)].v[idx.x];
    Vertex v1 = vertices[nonuniformEXT(mesh_id)].v[idx.y];
    Vertex v2 = vertices[nonuniformEXT(mesh_id)].v[idx.z];

    const vec3 barycentre = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 normal = normalize(v0.normal * barycentre.x + v1.normal * barycentre.y + v2.normal * barycentre.z);
    normal = normalize(vec3(scene.i[gl_InstanceCustomIndexEXT].inverted * vec4(normal, 0.0)));

    vec3 local_pos = v0.pos * barycentre.x + v1.pos * barycentre.y + v2.pos * barycentre.z;
    vec3 world_pos = vec3(scene.i[gl_InstanceCustomIndexEXT].transform * vec4(local_pos, 1.0));

    vec2 tex_coord = v0.tex_coord * barycentre.x + v1.tex_coord * barycentre.y + v2.tex_coord * barycentre.z;
    
    payload.direct_color = pbr(mat, tex_coord, normalize(cam.pos - world_pos), normal);

    float step_length = length(payload.ray_origin - world_pos);
    vec3 step_abs = exp(-DENSITY * step_length);
    vec3 step_col = (vec3(1.) - step_abs) * henyeyGreenstein(constants.light_dir, payload.ray_dir);
    payload.vol_abs *= step_abs;

    uint ray_flags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    float t_max = 1000.0;
    float t_min = 0.0;
    vec3 shadow_start = world_pos + normal * 0.0001;
    is_shadow = true;
    
    traceRayEXT(top_level_AS, ray_flags, 0xFF, 0, 0, 1, shadow_start, t_min, constants.light_dir, t_max, 1);

    if(is_shadow) {
        payload.direct_color *= mat.ao;
        payload.vol_col += step_col * payload.vol_abs * mat.ao;
    } else {
        payload.vol_col += step_col * payload.vol_abs * constants.light_color * constants.light_intensity;
    }

    payload.depth = 100;
}
