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
    uint emissive_texture;
};

layout(location = 0) in vec3 in_worldpos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 4) flat in uint material_id;
layout(location = 5) in vec4 in_shadow_map_texcoord;

layout (binding = 0) uniform CameraMatrices {
	mat4 proj;

	mat4 view;
	mat4 view_inverse;
	mat4 light_vp;
	vec3 view_pos;
	vec3 light_dir;
} cam;
layout(binding = 1) buffer Materials { Material m[]; } materials;
layout(binding = 2) uniform sampler2D textures[];
layout(binding = 3) uniform sampler2D shadow_map;

layout(location = 0) out vec4 out_color;

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
    vec2 pos = in_texcoord;
    float pixel_size = 1024;
    if(mat.base_color_texture < UINT_MAX) {
        pos = floor(in_texcoord * pixel_size) / pixel_size;
        diffuse = texture(textures[mat.base_color_texture], pos).rgb;
    } else if(mat.emissive_texture < UINT_MAX) {
        diffuse = texture(textures[mat.emissive_texture], pos).rgb;
    }

    return diffuse;
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

    float bias = ( 1 - NdotL) * 0.0001;
    float shadow = get_shadow(bias);

    float cam_distance = length(cam.view_pos - in_worldpos);
    float fog = pow(cam_distance / 20, 2);

    vec3 color = (ambient + shadow * diffuse);

    out_color = vec4(color, 1.0);
    //out_color = vec4(in_worldpos, 1.0);
}