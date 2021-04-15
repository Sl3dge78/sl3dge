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

typedef struct vec2 {
    alignas(4) float x;
    alignas(4) float y;
} vec2;

inline void vec2_print(vec2 *v) {
    SDL_Log("%f, %f", v->x, v->y);
}

typedef struct vec3 {
    alignas(4) float x;
    alignas(4) float y;
    alignas(4) float z;
} vec3;

typedef struct vec4 {
    alignas(4) float x;
    alignas(4) float y;
    alignas(4) float z;
    alignas(4) float w;
} vec4;

typedef struct quat {
    float x;
    float y;
    float z;
    float w;
} quat;

typedef union mat4 {
    float v[16];
    float m[4][4];
} mat4;

// =====================================================

vec3 operator+(vec3 l, const vec3 r) {
    l.x += r.x;
    l.y += r.y;
    l.z += r.z;
    return l;
};

vec3 operator-(vec3 l, const vec3 r) {
    l.x -= r.x;
    l.y -= r.y;
    l.z -= r.z;
    return l;
};

/// Y up
inline vec3 spherical_to_carthesian(const vec2 v) {
    vec3 result;
    
    result.x = cos(v.x) * sin(v.y);
    result.y = cos(v.y);
    result.z = sin(v.x) * sin(v.y);
    
    return result;
}

inline void vec3_print(const vec3 *v) {
    SDL_Log("%f, %f, %f", v->x, v->y, v->z);
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

vec3 vec3_normalize(const vec3 v) {
    
    float length = sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
    return vec3 { v.x / length, v.y / length, v.z / length };
    
}

vec3 vec3_cross(const vec3 a, const vec3 b) {
    vec3 result;
    
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    
    return result;
}

float vec3_dot(const vec3 a, const vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// =====================================================

void mat4_print(const mat4* mat) {
    
    SDL_Log("\n%#.1f, %#.1f, %#.1f, %#.1f\n%#.1f, %#.1f, %#.1f, %#.1f\n%#.1f, %#.1f, %#.1f, %#.1f\n%#.1f, %#.1f, %#.1f, %#.1f",
            mat->v[0],mat->v[1],mat->v[2],mat->v[3],
            mat->v[4],mat->v[5],mat->v[6],mat->v[7],
            mat->v[8],mat->v[9],mat->v[10],mat->v[11],
            mat->v[12],mat->v[13],mat->v[14],mat->v[15]);
}

mat4 mat4_mul(const mat4* a, const mat4* b) {
    
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
    
    const float tan_theta_2 = tan(PI/4.0f);
    
    result.m[0][0] = 1.0f / (aspect_ratio * tan_theta_2);
    result.m[1][1] = -1.0f / tan_theta_2;
    result.m[2][2] = (near + far) / (far - near);
    result.m[2][3] = 1.0f;
    result.m[3][2] = -(2.0f * near * far) / (far - near);
    result.m[3][3] = 1.0f;
    
    return result;
}

void mat4_translate(mat4 *mat, const vec3 vec) {
    mat->m[3][0] += vec.x;
    mat->m[3][1] += vec.y;
    mat->m[3][2] += vec.z;
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

mat4 mat4_look_at(vec3 target, vec3 eye, vec3 up) {
    vec3 z_axis = vec3_normalize(target - eye);
    vec3 x_axis = vec3_normalize(vec3_cross(up, z_axis));
    vec3 y_axis = vec3_cross(z_axis, x_axis);
    
    mat4 mat = mat4_identity();
    
    mat.m[0][0] = x_axis.x;
    mat.m[0][1] = y_axis.x;
    mat.m[0][2] = z_axis.x;
    mat.m[0][3] = 0;
    
    mat.m[1][0] = x_axis.y;
    mat.m[1][1] = y_axis.y;
    mat.m[1][2] = z_axis.y;
    mat.m[1][3] = 0;
    
    mat.m[2][0] = x_axis.z;
    mat.m[2][1] = y_axis.z;
    mat.m[2][2] = z_axis.z;
    mat.m[2][3] = 0;
    
    mat.m[3][0] = -vec3_dot(x_axis, eye);
    mat.m[3][1] = -vec3_dot(y_axis, eye);
    mat.m[3][2] = -vec3_dot(z_axis, eye);
    mat.m[3][3] = 1;
    
    return mat;
}

void trs_to_mat4(mat4 *dst, const vec3 *t, const quat *r, const vec3 *s) {
    
    const float sqx = 2.0f * r->x * r->x;
    const float sqy = 2.0f * r->y * r->y;
    const float sqz = 2.0f * r->z * r->z;
    const float sqw = 2.0f * r->w * r->w;
    
    const float xy = r->x * r->y;
    const float zw = r->z * r->w;
    
    const float xz = r->x * r->z;
    const float yw = r->y * r->w;
    
    const float yz = r->y * r->z;
    const float xw = r->x * r->w;
    
    dst->v[0] = (1 - sqy - sqz) * s->x;
    dst->v[1] = 2.0f * (xy + zw) * s->x;
    dst->v[2] = 2.0f * (xz - yw) * s->x;
    dst->v[3] = 0.0f;
    
    dst->v[4] = 2.0f * (xy - zw) * s->y;
    dst->v[5] = (1.0f - sqx - sqz) * s->y;
    dst->v[6] = 2.0f * (yz + xw) * s->y;
    dst->v[7] = 0.0f;
    
    dst->v[8] = 2.0f * (xz + yw) * s->z;
    dst->v[9] = 2.0f * (yz - xw) * s->z;
    dst->v[10] = (1.0f - sqx - sqy) * s->z;
    dst->v[11] = 0.0f;
    
    dst->v[12] = t->x;
    dst->v[13] = t->y;
    dst->v[14] = t->z;
    dst->v[15] = 1.0f;
    
}

// =====================================================

void quat_to_mat4(mat4 *dst, const quat *q) {
    
    const float sqx = 2.0f * q->x * q->x;
    const float sqy = 2.0f * q->y * q->y;
    const float sqz = 2.0f * q->z * q->z;
    const float sqw = 2.0f * q->w * q->w;
    
    const float xy = q->x * q->y;
    const float zw = q->z * q->w;
    
    const float xz = q->x * q->z;
    const float yw = q->y * q->w;
    
    const float yz = q->y * q->z;
    const float xw = q->x * q->w;
    
    //const float invs = 1.0f / (sqx + sqy + sqz + sqw);
    
    dst->v[0] = (1 - sqy - sqz);
    dst->v[1] = 2.0f * (xy + zw);
    dst->v[2] = 2.0f * (xz - yw);
    dst->v[3] = 0.0f;
    
    dst->v[4] = 2.0f * (xy - zw);
    dst->v[5] = (1.0f - sqx - sqz);
    dst->v[6] = 2.0f * (yz - xw);
    dst->v[7] = 0.0f;
    
    dst->v[8] = 2.0f * (xz + yw);
    dst->v[9] = 2.0f * (yz + xw);
    dst->v[10] = (1 - sqx - sqy);
    dst->v[11] = 0.0f;
    
    dst->v[12] = 0.0f;
    dst->v[13] = 0.0f;
    dst->v[14] = 0.0f;
    dst->v[15] = 1.0f;
}