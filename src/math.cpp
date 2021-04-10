#include <math.h>

/* 
=== TODO ===

 CRITICAL

 MAJOR

 BACKLOG

 - mat4 lookat

 IMPROVEMENTS

- quaternions

 IDEAS
 

*/


#define PI 3.1415926535897932384626433f

u32 aligned_size(const u32 value, const u32 alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

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

typedef struct quat {
    float x;
    float y;
    float z;
    float w;
} quat;

void vec3_add(vec3 *vec, const vec3 add) {
    vec->x += add.x;
    vec->y += add.y;
    vec->z += add.z;
}

void vec3_fmul(vec3 *vec, const float mul) {
    vec->x *= mul;
    vec->y *= mul;
    vec->z *= mul;
}

vec3 vec3_fmul(const vec3 vec, const float mul) {
    
    vec3 result = vec;
    
    result.x *= mul;
    result.y *= mul;
    result.z *= mul;
    
    return (result);
}

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

void mat4_translate(mat4 *mat, const vec3 vec) {
    mat->m[0][3] += vec.x;
    mat->m[1][3] += vec.y;
    mat->m[2][3] += vec.z;
}

void mat4_rotate_x(mat4 *mat, const float radians) {
    mat->m[0][0] = 1.0f;
    mat->m[1][1] = cos(radians);
    mat->m[1][2] = -sin(radians);
    mat->m[2][1] = sin(radians);
    mat->m[2][2] = cos(radians);
    mat->m[3][3] = 1.0f;
}

void mat4_rotate_y(mat4 *mat, const float radians) {
    mat->m[0][0] = cos(radians);
    mat->m[0][2] = sin(radians);
    mat->m[1][1] = 1.0f; 
    mat->m[2][0] = -sin(radians);
    mat->m[2][2] = cos(radians);
    mat->m[3][3] = 1.0f;
    
}

void mat4_rotate_z(mat4 *mat, const float radians) {
    mat->m[0][0] = cos(radians);
    mat->m[0][1] = -sin(radians);
    mat->m[1][0] = sin(radians);
    mat->m[1][1] = cos(radians);
    mat->m[2][2] = 1.0f;
    mat->m[3][3] = 1.0f;
}


void mat4_rotate_euler(mat4 *mat, const vec3 euler) {
    
    const float cx = cos(euler.x);
    const float sx = sin(euler.x);
    const float cy = cos(euler.y);
    const float sy = sin(euler.y);
    const float cz = cos(euler.z);
    const float sz = sin(euler.z);
    
    const float t01 = -sz*cx;
    const float t02 = sz*sx;
    const float t11 = cz*cx;
    const float t12 = cz*-sx;
    
    mat->m[0][0] = cz*cy + t02*-sy;
    mat->m[0][1] = t01;
    mat->m[0][2] = cz*sy + t02*cy;
    
    mat->m[1][0] = sz*cy + t12*-sy;
    mat->m[1][1] = t11;
    mat->m[1][2] = sz*sy + t12*cy;
    
    mat->m[2][0] = cx*-sy;
    mat->m[2][1] = sx;
    mat->m[2][2] = cx*cy;
}


