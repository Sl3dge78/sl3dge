
struct HitPayload {
	vec3 hit_value;
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
	uint texture_id;
};
