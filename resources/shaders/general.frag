#version 460
#extension GL_GOOGLE_include_directive  : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable

#include "shader_utils.glsl"
#define M_PI 3.1415926535897932384626433832795

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
layout(binding = 6, set = 0) uniform sampler2D textures[];
layout(binding = 7, set = 0) buffer Materials {Material m[];} materials;

layout(push_constant) uniform Constants {
    vec4 clear_color;
    vec3 light_dir;
    float light_intensity;
    vec3 light_color;
} constants;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in vec3 frag_pos;
layout(location = 3) flat in uint mat_id;

layout(location = 0) out vec4 out_color;

vec3 get_albedo(Material mat) {
    
    vec3 ret = mat.albedo;
    if(mat.albedo_texture_id >= 0) {
         ret *= texture(textures[mat.albedo_texture_id], tex_coords).xyz;
    }
    return ret;
}
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
vec3 pbr(Material mat) {

    vec3 albedo = get_albedo(mat);
    
    vec3 V = normalize(cam.pos - frag_pos);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, mat.albedo, mat.metallic);

    vec3 Lo = vec3(0.0);

    // for (int i = 0; i < 1 ; i++) {
        //Light light = lights.l[0];
        vec3 L;
        vec3 radiance;
       // if (light.type == 1) {
       //     L = normalize(light.vec - frag_pos);  
       //     float dist = length(light.vec - frag_pos);
        //    float attenuation = 1.0 / (dist * dist);
       //     radiance = light.color * light.intensity * attenuation;
        //} else if (light.type == 0) {
            L = normalize(constants.light_dir);
            radiance = constants.light_color * constants.light_intensity;
        //}
        vec3 H = normalize(V + L);
        vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);
        float NDF = distribution_ggx(normal, H, mat.roughness);
        float G = geometry_smith(normal, V, L, mat.roughness);

        vec3 numer = NDF * G * F;
        float denom = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0);
        vec3 specular = numer / max(denom, 0.001);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - mat.metallic;

        float NdotL = max(dot(normal, L), 0.0);

        vec3 rim_light = vec3(1.0) * rim(normal, V, mat.rim_pow, mat.rim_strength);

        // Main color
        vec3 val = (kD * albedo / M_PI + specular + rim_light) * radiance * NdotL;

        // Shadow
       // if(light.cast_shadows == 1) { 
            rayQueryEXT rayQuery;
            vec3 ray_orig = frag_pos + (normal * 0.01);
            rayQueryInitializeEXT(rayQuery, top_level_AS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, ray_orig, 0.01, L, 1000.0);
            while (rayQueryProceedEXT(rayQuery)) { }
            if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT) {
                val *= mat.ao;
            } 
        //}
        Lo += val;
    // }
    
    vec3 color = Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    return color;
}

vec4 blinn_phong() {
    
    vec3 light_pos = vec3(0.0, 0.0, 4.0);
    vec3 L = normalize(light_pos - frag_pos);

    Material mat = materials.m[mat_id];

    vec3 ambient = vec3(1.0) * mat.ao;

    float diff = max(dot(normal, L), 0.0);
    vec3 diffuse = mat.albedo * diff;
    
    vec3 V = normalize(cam.pos - frag_pos);
    vec3 reflect_dir = reflect(-L, normal);

    const float shininess = 0;
    const float specular_strength = 0;

    float spec = pow(max(dot(V, reflect_dir), 0.0), shininess);
    vec3 specular = specular_strength * spec * vec3(1.0);
    vec3 rim_light = vec3(1.0) * rim(normal, V, mat.rim_pow, mat.rim_strength);
    specular += rim_light;
    vec4 ret = vec4(ambient + diffuse + specular, 1.0);
    
    if(mat.albedo_texture_id >= 0) {
         ret *= texture(textures[mat.albedo_texture_id], tex_coords);
    }

    return ret;
}


void main() {

    //out_color = blinn_phong();
    out_color = vec4(pbr(materials.m[mat_id]), 1.0);
}

