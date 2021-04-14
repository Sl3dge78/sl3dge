#version 460

#define M_PI 3.1415926535897932384626433832795

struct Material {
	vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    float rim_pow;
    float rim_strength;
};

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 frag_pos;
layout(location = 2) in vec3 cam_pos;

layout(location = 0) out vec4 out_color;

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

    vec3 albedo = mat.albedo;
    
    vec3 V = normalize(cam_pos - frag_pos);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, mat.albedo, mat.metallic);

    vec3 Lo = vec3(0.0);
	
	vec3 light_dir = normalize(vec3(1, -1, 0.15));
	vec3 light_color = vec3(.99, .72, 0.07);
	float light_intensity = 5.0;

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
            L = normalize(light_dir);
            radiance = light_color * light_intensity;
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
		/*
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
		*/

        Lo += val;
    // }
    
    vec3 color = Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    return color;
}

void main() {
	
	Material mat;
	mat.albedo = vec3(1.0);
	mat.metallic = 0.0;
    mat.roughness = 0.0;
	mat.ao = 0.01;
    mat.rim_pow = 8;
    mat.rim_strength = 5.0;
	
	vec3 color = pbr(mat);

    out_color = vec4(color, 1.0);
}