#version 460
#extension GL_EXT_ray_tracing : require

struct Payload {
    vec3 direct_color;
};

layout(binding = 1, set = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT Payload payload;

void main() {
    payload.direct_color = vec3(0.1,0.1,0.1);
}