#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable

struct Material {
	float ambient_intensity;
    float shininess;
    float specular_strength;
	vec3 diffuse_color;
	int texture_id;
};
struct Instance {
	uint mesh_id;
	uint mat_id;
	mat4 transform;
	mat4 inverted;
};

layout(set = 0, binding = 1) uniform sampler2D textures[];
layout (set = 0, binding = 2) uniform accelerationStructureEXT topLevelAS;
layout (set = 0, binding = 3) buffer Scene {Instance i[];} scene;
layout (set = 0, binding = 4) buffer Materials {Material m[];} materials;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in vec3 frag_pos;
layout(location = 3) flat in uint mat_id;
layout(location = 4) flat in vec3 view_pos;

layout(location = 0) out vec4 out_color;

void main() {

    vec3 light_pos = vec3(0.0, 0.0, 2.0);
    vec3 L = normalize(light_pos - frag_pos);

    Material mat = materials.m[mat_id];

    vec3 ambient = vec3(1.0) * mat.ambient_intensity;

    float diff = max(dot(normal, L), 0.0);
    vec3 diffuse = mat.diffuse_color * diff;
    
    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 reflect_dir = reflect(-L, normal);

    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), mat.shininess);
    vec3 specular = mat.specular_strength * spec * vec3(1.0);

    out_color = vec4(ambient + diffuse /*+ specular*/, 1.0);
    if(mat.texture_id >= 0) {
        out_color *= texture(textures[mat.texture_id], tex_coords);
    }

    rayQueryEXT rayQuery;
    vec3 ray_orig = frag_pos + (normal * 0.01);
	rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, ray_orig, 0.01, L, 1000.0);

	while (rayQueryProceedEXT(rayQuery)) { }

	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT) {
		
        out_color *= 0.1;
	}
}
