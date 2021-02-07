
#define M_PI 3.1415926535897932384626433832795

#define ANISOTROPY .4
#define DENSITY vec3(.005, .005, .004)
#define DENSITY2 vec3(.0045, .0045, .006)
#define STEP_DIST 0.5

struct HitPayload {
    uint null;
    vec3 direct_color;
    uint seed;
	uint depth;
	vec3 ray_origin;
	vec3 ray_direction;
	vec3 weight;
};

struct HitPayloadSimple {
    uint null;
	vec3 direct_color;
	vec3 ray_dir;
	uint depth;
	vec3 pos;
	vec3 vol_col;
	vec3 vol_abs;
};

struct Vertex {
	vec3 pos;
	vec3 normal;
	vec2 tex_coord;
};

struct Instance {
	uint mesh_id;
	uint mat_id;
	mat4 transform;
	mat4 inverted;
};

struct Material {
	vec3 albedo;
    int albedo_texture_id;
    float metallic;
    float roughness;
    float ao;
    float rim_pow;
    float rim_strength;
};

struct Light {
    int type;
    vec3 color;
    float intensity;
    vec3 vec;
    int cast_shadows;
};

float henyeyGreenstein(vec3 dirI, vec3 dirO){
	float cosTheta = dot(dirI, dirO);
 	return M_PI/4.0 * (1.0 - ANISOTROPY * ANISOTROPY) / pow(1.0 + ANISOTROPY * ANISOTROPY - 2.0 * ANISOTROPY *cosTheta, 3.0/2.0);
}

