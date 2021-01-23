#version 460


layout(set = 0, binding = 0) uniform CameraMatrices {
    mat4 view;
    mat4 proj;
    mat4 view_inverse;
    mat4 proj_inverse;
    int frame;
} cam;


layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_tex_coords;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 tex_coords;
layout(location = 2) out vec3 frag_pos;


void main() {
    //gl_Position = cam.proj *  cam.view * mesh.transform * vec4(in_position, 1.0);
    gl_Position = cam.proj *  cam.view * vec4(in_position, 1.0);
    //frag_pos = vec3(mesh.transform * vec4(in_position, 1.0));
    frag_pos = in_position;
    normal = in_normal;
    tex_coords = in_tex_coords;

}
