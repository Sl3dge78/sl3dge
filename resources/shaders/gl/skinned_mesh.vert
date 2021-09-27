#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 joints;
layout (location = 4) in vec4 weights;

out vec3 Normal;
out vec2 TexCoord;
out vec4 shadow_map_texcoord;
out vec3 worldpos;

uniform mat4 transform;
uniform mat4 vp;
uniform mat4 light_matrix;
uniform mat4 joint_matrices[10];

void main() {
	mat4 skin_mat = weights.x * joint_matrices[int(joints.x)] + 
		weights.y * joint_matrices[int(joints.y)] +
		weights.z * joint_matrices[int(joints.z)] +
		weights.w * joint_matrices[int(joints.w)];

    vec4 pos = transform * skin_mat * vec4(aPos, 1.0);
    worldpos = pos.xyz;
    Normal = aNormal;
    TexCoord = aTexCoord;
    shadow_map_texcoord = light_matrix * pos;

	gl_Position = vp * pos;
}
