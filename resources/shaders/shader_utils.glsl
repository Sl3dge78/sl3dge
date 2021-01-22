struct HitPayload {
    uint null;
    vec3 direct_color;
    uint seed;
	uint depth;
	vec3 ray_origin;
	vec3 ray_direction;
	vec3 weight;
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
	float ambient_intensity;
	vec3 diffuse_color;
	int texture_id;
};

#define M_PI 3.1415926535897932384626433832795