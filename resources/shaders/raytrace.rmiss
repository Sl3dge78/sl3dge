#version 460

#extension GL_EXT_ray_tracing : require


void main() {


}

/*
void main() {

    uint ray_flags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;

    float t_max = 1000;
    is_shadow = true;
    traceRayEXT(top_level_AS, ray_flags, 0xFF, 0, 0, 1, payload.pos, 0.1, constants.light_dir, t_max, 1);

    vec3 step_abs = exp(-DENSITY  * STEP_DIST);
    vec3 step_col = (vec3(1.) - step_abs) * henyeyGreenstein(constants.light_dir, payload.ray_dir);
    payload.vol_abs *= step_abs;

    if(!is_shadow) {
        payload.vol_col += step_col * payload.vol_abs * constants.light_color * (constants.light_intensity);
    } else {
        payload.vol_col += step_col * payload.vol_abs;
    }
    payload.direct_color = constants.clear_color.xyz * .25;
}
*/