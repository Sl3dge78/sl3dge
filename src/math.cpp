#include <math.h>

// TODO LIST
//
// Matrix operations (rotate, look at, ...)
// TRS to mat4 ce serait chouette aussi

typedef struct vec4 {
    alignas(4) float x;
    alignas(4) float y;
    alignas(4) float z;
    alignas(4) float w;
} vec4;

typedef struct vec3 {
    alignas(4) float x;
    alignas(4) float y;
    alignas(4) float z;
} vec3;

typedef union mat4 {
    float v[16];
    float m[4][4];
} mat4;

void mat4_print(const mat4* mat) {
    
    SDL_Log("\n%#.1f, %#.1f, %#.1f, %#.1f\n%#.1f, %#.1f, %#.1f, %#.1f\n%#.1f, %#.1f, %#.1f, %#.1f\n%#.1f, %#.1f, %#.1f, %#.1f",
            mat->v[0],mat->v[1],mat->v[2],mat->v[3],
            mat->v[4],mat->v[5],mat->v[6],mat->v[7],
            mat->v[8],mat->v[9],mat->v[10],mat->v[11],
            mat->v[12],mat->v[13],mat->v[14],mat->v[15]);
}

mat4 mat4_mul(mat4* a, const mat4* b) {
    
    mat4 result = {};
    result.m[0][0] = a->m[0][0] * b->m[0][0] + a->m[0][1] * b->m[1][0] + a->m[0][2] * b->m[2][0] + a->m[0][3] * b->m[3][0];
    result.m[0][1] = a->m[0][0] * b->m[0][1] + a->m[0][1] * b->m[1][1] + a->m[0][2] * b->m[2][1] + a->m[0][3] * b->m[3][1];
    result.m[0][2] = a->m[0][0] * b->m[0][2] + a->m[0][1] * b->m[1][2] + a->m[0][2] * b->m[2][2] + a->m[0][3] * b->m[3][2];
    result.m[0][3] = a->m[0][0] * b->m[0][3] + a->m[0][1] * b->m[1][3] + a->m[0][2] * b->m[2][3] + a->m[0][3] * b->m[3][3];
    
    result.m[1][0] = a->m[1][0] * b->m[0][0] + a->m[1][1] * b->m[1][0] + a->m[1][2] * b->m[2][0] + a->m[1][3] * b->m[3][0];
    result.m[1][1] = a->m[1][0] * b->m[0][1] + a->m[1][1] * b->m[1][1] + a->m[1][2] * b->m[2][1] + a->m[1][3] * b->m[3][1];
    result.m[1][2] = a->m[1][0] * b->m[0][2] + a->m[1][1] * b->m[1][2] + a->m[1][2] * b->m[2][2] + a->m[1][3] * b->m[3][2];
    result.m[1][3] = a->m[1][0] * b->m[0][3] + a->m[1][1] * b->m[1][3] + a->m[1][2] * b->m[2][3] + a->m[1][3] * b->m[3][3];
    
    
    result.m[2][0] = a->m[2][0] * b->m[0][0] + a->m[2][1] * b->m[1][0] + a->m[2][2] * b->m[2][0] + a->m[2][3] * b->m[3][0];
    result.m[2][1] = a->m[2][0] * b->m[0][1] + a->m[2][1] * b->m[1][1] + a->m[2][2] * b->m[2][1] + a->m[2][3] * b->m[3][1];
    result.m[2][2] = a->m[2][0] * b->m[0][2] + a->m[2][1] * b->m[1][2] + a->m[2][2] * b->m[2][2] + a->m[2][3] * b->m[3][2];
    result.m[2][3] = a->m[2][0] * b->m[0][3] + a->m[2][1] * b->m[1][3] + a->m[2][2] * b->m[2][3] + a->m[2][3] * b->m[3][3];
    
    result.m[3][0] = a->m[3][0] * b->m[0][0] + a->m[3][1] * b->m[1][0] + a->m[3][2] * b->m[2][0] + a->m[3][3] * b->m[3][0];
    result.m[3][1] = a->m[3][0] * b->m[0][1] + a->m[3][1] * b->m[1][1] + a->m[3][2] * b->m[2][1] + a->m[3][3] * b->m[3][1];
    result.m[3][2] = a->m[3][0] * b->m[0][2] + a->m[3][1] * b->m[1][2] + a->m[3][2] * b->m[2][2] + a->m[3][3] * b->m[3][2];
    result.m[3][3] = a->m[3][0] * b->m[0][3] + a->m[3][1] * b->m[1][3] + a->m[3][2] * b->m[2][3] + a->m[3][3] * b->m[3][3];
    
    return result;
}

const mat4 mat4_identity() {
    mat4 result;
    
    result = {
        1.0f,0.0f,0.0f,0.0f,
        0.0f,1.0f,0.0f,0.0f,
        0.0f,0.0f,1.0f,0.0f,
        0.0f,0.0f,0.0f,1.0f
    };
    
    return result;
}

void mat4_transpose(mat4 *mat) {
    
    mat4 tmp = {};
    
    for(u32 c = 0; c < 4; c ++) {
        for(u32 r = 0; r < 4; r ++) {
            tmp.v[c * 4 + r] = mat->v[r * 4 + c];
        }
    }
    
    for(u32 i = 0; i < 16; i++) {
        mat->v[i] = tmp.v[i];
    }
}

mat4 mat4_perspective(const float fov, const float aspect_ratio, const float near, const float far) {
    
    mat4 result = {};
    
    const float tan_theta_2 = tan(fov/2.0f);
    
    result.m[0][0] = 1.0f / tan_theta_2;
    result.m[1][1] = aspect_ratio / tan_theta_2;
    result.m[2][2] = (near + far) / (near - far);
    result.m[2][3] = (2.0f * near * far) / (near - far);
    result.m[3][2] = -1.0f;
    
    return result;
}

void mat4_translate(mat4* mat, const vec3 vec) {
    mat->m[0][3] += vec.x;
    mat->m[1][3] += vec.y;
    mat->m[2][3] += vec.z;
}
/*
// TODO(Guigui):  Coords are inversed (x is y)
mat4 mat4_rotation_x(const float radians) {
    
    mat4 result = {};
    result.m[0][0] = 1.0f;
    result.m[1][1] = cos(radians);
    result.m[2][1] = -sin(radians);
    result.m[1][2] = sin(radians);
    result.m[2][2] = cos(radians);
    result.m[3][3] = 1.0f;
    return result;
}

mat4 mat4_rotation_y(const float radians) {
    
    mat4 result = {};
    result.m[0][0] = cos(radians);
    result.m[2][0] = sin(radians);
    result.m[1][1] = 1.0f; 
    result.m[0][2] = -sin(radians);
    result.m[2][2] = cos(radians);
    result.m[3][3] = 1.0f;
    return result;
}

mat4 mat4_rotation_z(const float radians) {
    
    mat4 result = {};
    result.m[0][0] = cos(radians);
    result.m[1][0] = -sin(radians);
    result.m[0][1] = sin(radians);
    result.m[1][1] = cos(radians);
    result.m[2][2] = 1.0f;
    result.m[3][3] = 1.0f;
    return result;
}
*/