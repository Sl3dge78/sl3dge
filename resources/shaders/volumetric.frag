#version 460

#define M_PI 3.141592653589793



layout (binding = 0) uniform CameraMatrices {
	mat4 proj;
    mat4 proj_inverse;
	mat4 view;
	mat4 view_inverse;
	mat4 light_vp;
	vec3 view_pos;
	vec3 light_dir;
} cam;
layout(binding = 1) uniform sampler2D depth_map;
layout(binding = 2) uniform sampler2D shadow_map;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 out_color;

float Anisotropy = .4;
vec3 Density = vec3(.005, .005, .004);

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

float henyey_greenstein(vec3 diri, vec3 diro) {
    float cos_theta = dot(diri, diro);
    return M_PI/4.0 * (1.0-Anisotropy*Anisotropy) / pow(1.0 + Anisotropy*Anisotropy - 2.0*Anisotropy*cos_theta, 3.0/2.0);
}

vec3 calculate_world_position(vec2 uv, float depth) {
    vec3 fragment_view_pos = vec3(uv, depth);
    vec4 view_space_pos = cam.proj_inverse * vec4(fragment_view_pos, 1.0);
    view_space_pos /= view_space_pos.w;
    vec4 world_space_pos = cam.view_inverse * view_space_pos;
    return world_space_pos.xyz;
}

vec3 volumetric_fog(vec3 L, vec3 frag_worldpos) {

    int steps = 16;
    vec3 start = cam.view_pos;

    vec3 dir = normalize(frag_worldpos - cam.view_pos);
    float frag_distance = length(frag_worldpos - cam.view_pos);
    float total_dist = min(frag_distance, 5.0);

    vec3 ray_step = dir * (total_dist) / steps;

    vec3 step_abs = exp(-Density * total_dist);
    vec3 step_color = (vec3(1.0) - step_abs) * henyey_greenstein(-L, dir);
    vec3 vol_abs = vec3(1.0);

    vec3 rnd_v = (frag_worldpos + cam.view_pos) * 100.0;
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

void main() {

    float depth = texture(depth_map, uv).x;
    vec3 frag_worldpos = calculate_world_position(uv, depth);

    //out_color = vec4(volumetric_fog(cam.light_dir, frag_worldpos), 0);

    out_color = vec4(frag_worldpos,1);
    //out_color = vec4(0, 0, 0, 0);
}