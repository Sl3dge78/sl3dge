#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shader_clock : enable

#define M_PI 3.141592653589793
#define UINT_MAX 4294967295

struct Material {
    vec3 base_color;
    uint base_color_texture;
    uint metallic_roughness_texture;
    float metallic_factor;
    float roughness_factor;
    uint normal_texture;
    uint ao_texture;
};

layout(location = 0) in vec3 in_worldpos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 4) flat in uint material_id;
layout(location = 5) in vec4 in_shadow_map_texcoord;

layout (binding = 0) uniform CameraMatrices {
	mat4 proj;
	mat4 view;
	mat4 light_vp;
	vec3 view_pos;
	vec3 light_dir;
} cam;
layout(binding = 1) buffer Materials { Material m[]; } materials;
layout(binding = 2) uniform sampler2D textures[];
layout(binding = 3) uniform sampler2D shadow_map;

layout(location = 0) out vec4 out_color;

// RNG


uint tea(uint val0, uint val1) {
  uint v0 = val0;
  uint v1 = val1;
  uint s0 = 0;

  for(uint n = 0; n < 16; n++) {
    s0 += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
  }
  return v0;
}
uint lcg(inout uint prev) {
  uint LCG_A = 1664525u;
  uint LCG_C = 1013904223u;
  prev       = (LCG_A * prev + LCG_C);
  return prev & 0x00FFFFFF;
}
float rnd(inout uint prev) {
  return (float(lcg(prev)) / float(0x01000000));
}

vec3 get_normal(Material mat) {
    return in_normal.xyz;
    if(mat.normal_texture < UINT_MAX) {
        vec3 tangent = texture(textures[mat.normal_texture], in_texcoord).xyz * 2.0 - 1.0;

        vec3 q1 = dFdx(in_worldpos);
        vec3 q2 = dFdy(in_worldpos);
        vec2 st1 = dFdx(in_texcoord);
        vec2 st2 = dFdx(in_texcoord);

        vec3 N = normalize (in_normal.xyz);
        vec3 T = normalize( q1 * st2.t - q2 * st1.t);
        vec3 B = -normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);
        return normalize(TBN * tangent);
    } else {
        return in_normal.xyz;
    }
}
float get_shadow(float bias) {
    float shadow = 1.0;
    vec4 proj_coords = in_shadow_map_texcoord / in_shadow_map_texcoord.w;
    float current_depth = proj_coords.z;
    proj_coords = proj_coords * 0.5 + 0.5;
    
    if(1 == 0) {
        float closest_depth = texture(shadow_map, proj_coords.xy).r;
        shadow = closest_depth > current_depth - bias ? 1.0 : 0.1;        
    } else {
        shadow = 0.0;
        vec2 tex_dim = 1.0 / textureSize(shadow_map, 0);
        float scale = 1.0;
        int samples = int(scale);
        int count = 0;

        for(int x = -samples; x <= samples; ++x) {
            for(int y = -samples; y <= samples; ++y) {
                float closest_depth = texture(shadow_map, proj_coords.xy + vec2(x, y) * tex_dim).r; 
                shadow += closest_depth > current_depth - bias ? 1.0 : 0.0;        
                count ++;
            }    
        }
        shadow /= count;
    }
    return shadow;
}
vec3 base_color(Material mat) {

    const int steps = 4;
    float ambient = 0.1;
    float factor = 1.6;

    vec3 diffuse = mat.base_color;
    float pixel_size = 32;
    if(mat.base_color_texture < UINT_MAX) {
        vec2 pos = floor(in_texcoord * pixel_size) / pixel_size;
        diffuse = texture(textures[mat.base_color_texture], pos).rgb;
    }      
    
    return diffuse;
}
float Anisotropy = .4;
vec3 Density = vec3(.005, .005, .004);

float henyey_greenstein(vec3 diri, vec3 diro) {
    float cos_theta = dot(diri, diro);
    return M_PI/4.0 * (1.0-Anisotropy*Anisotropy) / pow(1.0 + Anisotropy*Anisotropy - 2.0*Anisotropy*cos_theta, 3.0/2.0);
}
vec3 volumetric_fog(vec3 L) {

    int steps = 16;
    vec3 start = cam.view_pos;

    vec3 dir = normalize(in_worldpos - cam.view_pos);
    float frag_distance = length(in_worldpos - cam.view_pos);
    float total_dist = min(frag_distance, 5.0);

    vec3 ray_step = dir * (total_dist) / steps;

    vec3 step_abs = exp(-Density * total_dist);
    vec3 step_color = (vec3(1.0) - step_abs) * henyey_greenstein(-L, dir);
    vec3 vol_abs = vec3(1.0);

    vec3 rnd_v = (in_worldpos + cam.view_pos) * 100.0;
    float a = (rnd_v.x * rnd_v.y * rnd_v.z) / 100.0;
    uint seed = tea(0, int(a));

    vec3 current_position = start + (ray_step * rnd(seed));

    vec3 accum = vec3(0.0);
    for (int i = 0; i < steps; i++) {
        
        vec4 shadow_coords = cam.light_vp * vec4(current_position, 1.0);
        vec4 proj_coords = shadow_coords / shadow_coords.w;
        float current_depth = proj_coords.z;

        proj_coords = proj_coords * 0.5 + 0.5;
        float closest_depth = texture(shadow_map, proj_coords.xy).r;
        vol_abs *= step_abs;
        if(closest_depth > current_depth){          
            accum += step_color * vol_abs;
        }

        current_position += ray_step;
    }
    return accum;
}

vec3 skycolor = vec3(0.53, 0.80, 0.92);
float ambient_intensity = 0.25;

void main() {
	
    Material mat = materials.m[material_id];

    vec3 N = get_normal(mat);
    vec3 L = cam.light_dir;
    float NdotL = max(dot(N, L), 0.0);

    vec3 base_color = base_color(mat);
    
    vec3 diffuse = base_color;

    vec3 ambient = skycolor * ambient_intensity * base_color;

    float bias = ( 1 - NdotL) * 0.0005;
    float shadow = get_shadow(bias);
    
    float cam_distance = length(cam.view_pos - in_worldpos);
    float fog = pow(cam_distance / 20, 2);

    vec3 color = (ambient + shadow * diffuse);
    color += volumetric_fog(L);
    
    out_color = vec4(color, 1.0);
}