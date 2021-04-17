#version 460

#extension GL_EXT_nonuniform_qualifier : enable

#define M_PI 3.141592653589793
#define UINT_MAX 4294967295

struct Material {
    vec3 albedo;
    uint base_color_texture;
    uint metallic_roughness_texture;
    float metallic_factor;
    float roughness_factor;
    uint normal_texture;
    /*
    float ao;
    float rim_pow;
    float rim_strength;
    */
};

layout(location = 0) in vec3 worldpos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec3 cam_pos;
layout(location = 4) flat in uint material_id;

layout(location = 0) out vec4 out_color;

layout(binding = 1) buffer Materials { Material m[]; } materials;
layout(binding = 2) uniform sampler2D textures[];

/*
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

    vec3 V = normalize(cam_pos - frag_pos);
    vec3 N = normalize(normal);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, mat.albedo, mat.metallic);

    vec3 Lo = vec3(0.0);
	
	//vec3 light_color = vec3(.99, .72, 0.07);
    vec3 light_color = vec3(1.0, 1.0, 1.0);
    vec3 light_dir = normalize(vec3(0, 0, -1));
	float attenuation = 5.0;

    vec3 L = normalize(-light_dir);
    vec3 H = normalize(V + L);
    vec3 radiance = light_color;
    
    float NDF = distribution_ggx(N, H, mat.roughness);
    float G = geometry_smith(N, V, L, mat.roughness);
    vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - mat.metallic;        

    float NdotL = max(dot(normal, L), 0.0);

    vec3 numer = NDF * G * F;
    float denom = 4.0 * max(dot(N, V), 0.0) * NdotL;
    vec3 specular = numer / max(denom, 0.001);
    
    //vec3 rim_light = vec3(1.0) * rim(normal, V, mat.rim_pow, mat.rim_strength);
    vec3 rim_light = vec3(0.0);

    // Main color
    //vec3 val = (kD * mat.albedo / M_PI + specular + rim_light) * radiance * NdotL;           
    Lo += (kD * mat.albedo / M_PI + specular) * radiance * NdotL;
    
    
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
    

    float ao = 0;

    vec3 ambient = vec3(0.03) * mat.albedo * ao;
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    return color;
}
*/
vec3 specular_reflection(float VdotH, vec3 r0, vec3 r90) {
    return r0 + (r90 - r0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

float geometric_occlusion(float NdotL, float NdotV, float r) {
    float attenuation_l = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuation_v = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
    return attenuation_l * attenuation_v;
}

float microfacet_distribution(float NdotH, float r) {
    float r2 = r * r;
    float f = (NdotH * r2 - NdotH) * NdotH + 1.0;
    return r2/(M_PI * f * f);
}

vec3 diffuse(vec3 diffuse_color) {
    return diffuse_color / M_PI;
}

vec3 get_ibl_contribution() {

    return vec3(0.0);
}

vec3 get_base_color(Material mat) {

    if(mat.base_color_texture < UINT_MAX) {
        return texture(textures[mat.base_color_texture], texcoord).xyz * mat.albedo;
    } else {
        return mat.albedo;
    }
}

vec3 get_normal(Material mat) {

    if(mat.normal_texture < UINT_MAX) {
        vec3 tangent = texture(textures[mat.normal_texture], texcoord).xyz * 2.0 - 1.0;
        vec3 q1 = dFdx(worldpos);
        vec3 q2 = dFdy(worldpos);
        vec2 st1 = dFdx(texcoord);
        vec2 st2 = dFdx(texcoord);

        vec3 N = normalize (in_normal);
        vec3 T = normalize( q1 * st2.t - q2 * st1.t);
        vec3 B = -normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);
        return normalize(TBN * tangent);
    } else {
        return in_normal;
    }
   
}

vec3 pbr2(Material mat) {

    vec3 f0 = vec3(0.04);

    vec3 diffuse_color = get_base_color(mat) * (vec3(1.0) - f0);

    float metallic = mat.metallic_factor;
    float roughness = mat.roughness_factor;

    if(mat.metallic_roughness_texture < UINT_MAX) {
        vec3 tex = texture(textures[mat.base_color_texture], texcoord).xyz;
        roughness *= tex.r;
        metallic *= tex.b;
    } 

    diffuse_color *= 1.0 - metallic; 

    float alpha_roughness = roughness * roughness;
    vec3 specular_color = mix(f0, mat.albedo.rgb, metallic);

    float reflectance = max(max(specular_color.r, specular_color.g), specular_color.b);
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    vec3 specular_environment_R0 = specular_color.rgb;
    vec3 specular_environment_R90 = vec3(1.0) * reflectance90;

    vec3 light_dir = normalize(vec3(1, 1, -1));

    vec3 N = get_normal(mat);
    //return N;
    vec3 V = normalize(cam_pos - worldpos);
    vec3 L = normalize(light_dir);
    vec3 H = normalize(L+V);
    vec3 reflection = -normalize(reflect(V, N));
    reflection.y *= -1.0;

    float NdotL = clamp(dot(N, L), 0.001, 1.0);
    float NdotV = clamp(abs(dot(N, V)), 0.001, 1.0);
    //float NdotV = clamp(dot(N, V), 0.1, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float LdotH = clamp(dot(L, H), 0.0, 1.0);
    float VdotH = clamp(dot(V, H), 0.0, 1.0);

    vec3 F = specular_reflection(VdotH, specular_environment_R0, specular_environment_R90);
    float G = geometric_occlusion(NdotL, NdotV, alpha_roughness);
    float D = microfacet_distribution(NdotH, alpha_roughness);

    const vec3 light_color = vec3(5.0);

    vec3 diffuse_contrib = (1.0 - F) * diffuse(diffuse_color);
    vec3 spec_contrib = F* G * D / (4.0 * NdotL * NdotV);

    vec3 color = NdotL * light_color * (diffuse_contrib + spec_contrib);

    color += get_ibl_contribution();

    return color;
}

void main() {
	
	vec3 color = pbr2(materials.m[material_id]);

    uint texture_id = materials.m[material_id].base_color_texture;
    //color = texture(textures[texture_id], texcoord).xyz;
    
    out_color = vec4(color, 1.0);
}