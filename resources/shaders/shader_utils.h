
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
	mat4 transform;
	mat4 inverted;
};
