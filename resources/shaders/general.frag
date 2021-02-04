#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable

#define M_PI 3.1415926535897932384626433832795

struct Material {
	vec3 albedo;
    int albedo_texture_id;
    float metallic;
    float roughness;
    float ao;
};
struct Instance {
	uint mesh_id;
	uint mat_id;
	mat4 transform;
	mat4 inverted;
};

layout(set = 0, binding = 1) uniform sampler2D textures[];
layout (set = 0, binding = 2) uniform accelerationStructureEXT topLevelAS;
layout (set = 0, binding = 3) buffer Scene {Instance i[];} scene;
layout (set = 0, binding = 4) buffer Materials {Material m[];} materials;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in vec3 frag_pos;
layout(location = 3) flat in uint mat_id;

layout(push_constant) uniform Constants {vec3 view_pos; } constants;

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
vec3 pbr(Material mat) {

    vec3 albedo = get_albedo(mat);
    vec3 light_pos = vec3(0.0, 0.0, 4.0);
    vec3 light_color = vec3(23.47, 21.31, 20.79);
    
    vec3 V = normalize(constants.view_pos - frag_pos);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, mat.albedo, mat.metallic);

    vec3 Lo = vec3(0.0);
    // TODO : for each light
    //for (int i = 0; i < light_amount ; i++) {
       
    //}
    vec3 L = normalize(light_pos - frag_pos);
    vec3 H = normalize(V + L);
    
    rayQueryEXT rayQuery;
    vec3 ray_orig = frag_pos + (normal * 0.01);
	rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, ray_orig, 0.01, L, 1000.0);
	while (rayQueryProceedEXT(rayQuery)) { }
	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT) {
        Lo += 0.0;
	} else {
        float dist = length(light_pos - frag_pos);
        float attenuation = 1.0 / (dist * dist);
        vec3 radiance = light_color * attenuation;
        
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
        Lo += (kD * albedo / M_PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * mat.ao;
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    return color;
}
/*
vec4 blinn_phong() {
    
    vec3 light_pos = vec3(0.0, 0.0, 2.0);
    vec3 L = normalize(light_pos - frag_pos);

    Material mat = materials.m[mat_id];

    vec3 ambient = vec3(1.0) * mat.ao;

    float diff = max(dot(normal, L), 0.0);
    vec3 diffuse = mat.albedo * diff;
    
    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 reflect_dir = reflect(-L, normal);

    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), mat.shininess);
    vec3 specular = mat.specular_strength * spec * vec3(1.0);
    vec4 ret = vec4(ambient + diffuse + specular, 1.0);
    
    if(mat.texture_id >= 0) {
         ret *= texture(textures[mat.texture_id], tex_coords);
    }
}
*/

void main() {

    //out_color = blinn_phong();
    out_color = vec4(pbr(materials.m[mat_id]), 1.0);
}
