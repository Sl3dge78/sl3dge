#version 410

in vec3 Normal;
in vec2 TexCoord;
in vec4 shadow_map_texcoord;
in vec2 worldpos;

out vec4 FragColor;

uniform sampler2D diffuse;
uniform vec3 diffuse_color;

uniform sampler2D shadow_map;
uniform vec3 light_dir;

float get_shadow(double bias) {

    float shadow = 0.0;
    vec3 proj_coords = shadow_map_texcoord.xyz / shadow_map_texcoord.w;
    proj_coords = proj_coords * 0.5 + 0.5;
	if (proj_coords.x > 1.0 || proj_coords.x < 0.0 || proj_coords.y < 0.0 || proj_coords.y > 1.0) {
		return 1.0;
	}
    float current_depth = proj_coords.z;

    vec2 tex_dim = 1.0 / textureSize(shadow_map, 0);
    float scale = 4.0;
    int samples = int(scale);
    int count = 0;

    for(int x = -samples; x <= samples; ++x) {
        for(int y = -samples; y <= samples; ++y) {
            vec2 uv = proj_coords.xy + vec2(x, y) * tex_dim; 
            float closest_depth = texture(shadow_map, uv).r;
			if(closest_depth > 0.9999999) {
				return 1.0;
			}
            shadow += closest_depth > current_depth - bias ? 1.0 : 0.1;
            count ++;
        }
    }
	shadow /= count;

    return shadow;
}

void main() {
    float NdotL = dot(Normal, -light_dir);
	NdotL = clamp(NdotL, 0.0, 1.0);
    vec3 base_color = texture(diffuse, TexCoord).rgb * diffuse_color;
	
    double bias = 0.003 * tan(acos(NdotL));
	bias = clamp(bias, 0.0, 0.0005);
    float shadow = get_shadow(bias);
	
	// This allows us to get a nice blend between shadows and ndotl, avoids the double darkness that we can get sometimes...
	shadow *= NdotL;
	shadow = clamp(shadow, 0.1, 1.0);

    float ambient = 0.5;
    FragColor = vec4((shadow + ambient) * base_color, 1.0);
}
