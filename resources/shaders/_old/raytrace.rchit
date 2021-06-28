#version 460

#extension GL_EXT_ray_tracing : require

struct HitPayload {
    vec3 direct_color;
};

layout(location = 0) rayPayloadInEXT HitPayload payload;

void main() {
    payload.direct_color = vec3(0.7, 0.75, 0.75);

}
