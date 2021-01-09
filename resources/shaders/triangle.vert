#version 450


layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
} scene;

layout(set = 1, binding = 0) uniform MeshUBO {
    mat4 transform;
} mesh;


layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coords;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 tex_coords;
layout(location = 2) out vec3 frag_pos;


void main() {
    gl_Position = scene.proj *  scene.view * mesh.transform * vec4(in_position, 1.0);
    frag_pos = vec3(mesh.transform * vec4(in_position, 1.0));
    normal = in_normal;
    tex_coords = in_tex_coords;

}
