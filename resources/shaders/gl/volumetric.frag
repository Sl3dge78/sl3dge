#version 460 core
#define M_PI 3.141592653589793

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D shadow_map;
uniform sampler2D depth_map;
uniform sampler2D screen_texture;
uniform mat4 proj_inverse;
uniform mat4 view_inverse;
uniform mat4 light_matrix;
uniform vec3 view_pos;
uniform vec3 light_dir;

float Anisotropy = 0.8;
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

float factor = 2;
float div = 3.0/2.0;

float henyey_greenstein(vec3 diri, vec3 diro) {
    float cos_theta = dot(diri, diro);
    return M_PI/4.0 * (1.0-Anisotropy*Anisotropy) / pow(1.0 + Anisotropy*Anisotropy - factor*Anisotropy*cos_theta, div);
}

vec3 calculate_world_position(vec2 uv, float depth) {
    vec4 fragment_view_pos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view_space_pos = proj_inverse * fragment_view_pos;
    view_space_pos /= view_space_pos.w;
    vec4 world_space_pos = view_inverse * view_space_pos;
    return world_space_pos.xyz;
}

vec3 light_color = vec3(0.99, .72, 0.07);
float light_intensity = 5.0f;

vec3 volumetric_fog(vec3 L, vec3 frag_worldpos) {

    //int steps = 32;
    vec3 start = view_pos;

    vec3 dir = normalize(frag_worldpos - view_pos);
    float frag_distance = length(frag_worldpos - view_pos);

    float max_step_length = 0.5;
    float total_dist = min(frag_distance, 500);

    float step_length = .5;
    int step_amount = int(total_dist / step_length);
    vec3 ray_step = dir * step_length;

    vec3 step_abs = exp(-Density * step_length);
    vec3 step_color = (vec3(1.0) - step_abs) * henyey_greenstein(-L, dir);
    vec3 vol_abs = vec3(0.8);

    vec3 rnd_v = (frag_worldpos + view_pos) * 100.0;
    float a = (rnd_v.x * rnd_v.y * rnd_v.z) / 100.0;
    uint seed = tea(0, int(a));

    vec3 current_position = start + (ray_step * rnd(seed));

    vec3 accum = vec3(0.0);
    for (int i = 0; i < step_amount; i++) {

        vec4 shadow_coords = light_matrix * vec4(current_position, 1.0);
        vec4 proj_coords = shadow_coords / shadow_coords.w;
        proj_coords = proj_coords * 0.5 + 0.5;
        float current_depth = proj_coords.z;
		float closest_depth = 0.0f;
		if (proj_coords.x > 1.0 || proj_coords.x < 0.0 || proj_coords.y < 0.0 || proj_coords.y > 1.0) {
			closest_depth = 0.0;
		} else {
        	closest_depth = texture(shadow_map, proj_coords.xy).r;
		}
        vol_abs *= step_abs;
        if(closest_depth > current_depth){ // ! shadow
            accum += step_color * vol_abs * light_intensity * light_color;
        } else { // shadow
            accum += step_color * vol_abs * .001;
        }

        current_position += ray_step;
    }
    return accum * vol_abs;
}

void main() {

    float depth = texture(depth_map, TexCoord).x;
    vec3 frag_worldpos = calculate_world_position(TexCoord, depth);
    FragColor = texture(screen_texture, TexCoord) + vec4(volumetric_fog(light_dir, frag_worldpos), 1.0);

}