#pragma once

#include <math.h>
#include <stdalign.h>

#include "sTypes.h"
#include "sLogging.h"

#define PI 3.1415926535897932384626433f
#define MAX(x, y) x > y ? x : y
#define MIN(x, y) x < y ? x : y

typedef struct Rect {
    i32 x;
    i32 y;
    i32 w;
    i32 h;
} Rect;

typedef struct Vec2f {
    alignas(4) f32 x;
    alignas(4) f32 y;
} Vec2f;

typedef Vec2f Vec2;

typedef struct Vec3 {
    alignas(4) f32 x;
    alignas(4) f32 y;
    alignas(4) f32 z;
} Vec3;

typedef struct Vec4 {
    alignas(4) f32 x;
    alignas(4) f32 y;
    alignas(4) f32 z;
    alignas(4) f32 w;
} Vec4;

typedef struct Quat {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} Quat;

typedef struct Transform {
    Vec3 translation;
    Quat rotation;
    Vec3 scale;
} Transform;

typedef f32 Mat4[16];

// --------
// DECLARATIONS


// --------
// GENERAL

inline f32 lerp(f32 a, f32 b, f32 t);

// --------
// MAT4

void mat4_identity(f32 *result);
void mat4_perspective(const float fov,const float aspect_ratio,const float near_,const float far_, f32 *result);
void mat4_perspective_gl(const f32 fov, const f32 aspect_ratio, const f32 znear, const f32 zfar, f32 *result);
void mat4_ortho(const float t,const float b,const float l,const float r,const float znear,const float zfar, f32 *result);
void mat4_ortho_gl(const float t,const float b,const float l,const float r,const float znear,const float zfar,f32 *result);
void mat4_ortho_zoom(float ratio, float zoom, float n, float f, f32 *result);
void mat4_ortho_zoom_gl(float ratio, float zoom, float n, float f, f32 *result);
void mat4_look_at(const Vec3 target, const Vec3 eye, const Vec3 up, f32 *mat);

inline void trs_quat_to_mat4(const Vec3 *t, const Quat *r, const Vec3 *s, f32 *restrict dst);
void trs_to_mat4(const Vec3 t, const Vec3 r, const Vec3 s, f32 *result);

void mat4_transpose(Mat4 *restrict mat);
void mat4_mul(const f32 *restrict const a, const f32 *restrict const b, f32 *result);
Vec4 mat4_mul_vec4(const f32 *restrict const mat, const Vec4 vec);
Vec3 mat4_mul_vec3(const f32 *const mat, const Vec3 vec);
void mat4_inverse(const f32 *const restrict m, f32 *out);

Vec3 mat4_get_translation(const Mat4 mat);
inline void mat4_translateby(f32 *mat, const Vec3 vec);
inline void mat4_set_position(f32 *mat, const Vec3 vec);

void mat4_rotation_x(f32 *mat, const float radians);
void mat4_rotation_y(f32 *mat, const float radians);
void mat4_rotation_z(f32 *mat, const float radians);
void mat4_rotate_euler(f32 *restrict mat, const Vec3 euler);

Vec3 mat4_get_scale(const f32 *mat);
void mat4_scaleby(f32 *restrict dst, const Vec3 s);

void mat4_print(const f32 *const mat);

// --------
// QUAT


Quat mat4_get_rotation(f32 *mat);
void quat_to_mat4(f32 *dst, const Quat *q);
Quat quat_from_axis(const Vec3 axis, const f32 angle);
Quat quat_identity();
inline Quat quat_normalize(Quat q);
Quat quat_nlerp(const Quat a, const Quat b, const f32 t);
Quat quat_slerp(const Quat, const Quat, const f32);
void quat_sprint(const Quat q, char *buf);

// --------
// TRANSFORM

inline void transform_identity(Transform *xform) ;
inline void transform_to_mat4(const Transform *xform, Mat4* mat);
inline void mat4_to_transform(const Mat4 *mat, Transform *xform) ;

// -----------
// Bit operations

u32 aligned_size(const u32 value, const u32 alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

static inline f32 radians(const f32 angle) {
    return angle / 180.f * PI;
}

u32 swap_u32(const u32 val) {
    u32 result = ((val >> 8) & 0x0000FF00) | ((val << 8) & 0x00FF0000) |
        ((val >> 24) & 0x000000FF) | ((val << 24) & 0xFF000000);
    return result;
}

u16 swap_u16(const u16 val) {
    u16 result = ((val << 8) & 0xFF00) | ((val >> 8) & 0x00FF);
    return result;
}

u8 swap_u8(u8 val) {
    u8 result = 0;
    for(u8 i = 0; i < 8; ++i) {
        result <<= 1;
        u8 bit = val & (1 << i);
        result |= (bit > 0);
    }
    return result;
}

// =====================================================
// VEC 2

inline void vec2f_print(const Vec2f v) {
    sLog("%f, %f", v.x, v.y);
}

static inline f32 vec2f_length(const Vec2f v) {
    return sqrt((v.x * v.x) + (v.y * v.y));
}

void vec2f_normalize(Vec2f *v) {
    const f32 length = vec2f_length(*v);
    
    v->x /= length;
    v->y /= length;
}

inline bool vec2f_in_circle(const Vec2f v, const Vec2f center, f32 radius) {
    return v.x > center.x - radius && v.x < center.x + radius && v.y > center.y - radius &&
        v.y < center.y + radius;
}

inline bool vec2f_in_rect(const Vec2f v, const Rect rect) {
    return (v.x > rect.x && v.y > rect.y && v.x < rect.w + rect.x && v.y < rect.h + rect.y);
}

inline Vec2f vec2_sub(const Vec2f r, const Vec2f l) {
    Vec2f result = {r.x - l.x, r.y - l.y};
    return result;
}

inline void vec2_ssub(Vec2f *v, const Vec2f b) {
    v->x -= b.x;
    v->y -= b.y;
}

static inline f32 vec2f_distance(const Vec2f a, const Vec2f b) {
    Vec2f diff = vec2_sub(a, b);
    return vec2f_length(diff);
}

// =====================================================
// VEC 3


/// Y up
inline Vec3 spherical_to_carthesian(const Vec2f v) {
    Vec3 result;
    const Vec2f o = {v.x - (PI / 2.0f), v.y + (PI / 2.0f)};
    result.x = cos(o.x) * sin(o.y);
    result.y = cos(o.y);
    result.z = sin(o.x) * sin(o.y);
    
    return result;
}

inline void vec3_print(const Vec3 *v) {
    sLog("%f, %f, %f", v->x, v->y, v->z);
}

Vec3 vec3_add(const Vec3 a, const Vec3 b) {
    Vec3 result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return result;
}

Vec3 vec3_sub(const Vec3 a, const Vec3 b) {
    Vec3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}

Vec3 vec3_mul(const Vec3 a, const Vec3 b) {
    Vec3 result = {a.x * b.x, a.y * b.y, a.z * b.z};
    return result;
}

Vec3 vec3_fmul(const Vec3 vec, const float mul) {
    Vec3 result = vec;
    
    result.x *= mul;
    result.y *= mul;
    result.z *= mul;
    
    return (result);
}

f32 vec3_length(const Vec3 v) {
    return sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
}

Vec3 vec3_normalize(const Vec3 v) {
    f32 length = vec3_length(v);
    Vec3 result = {v.x / length, v.y / length, v.z / length};
    return result;
}

void vec3_normalize2(const Vec3 v, Vec3 *result) {
    f32 length = vec3_length(v);
    result->x = v.x / length;
    result->y = v.y / length;
    result->z = v.z / length;
}

Vec3 vec3_cross(const Vec3 a, const Vec3 b) {
    Vec3 result;
    
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    
    return result;
}

f32 vec3_dot(const Vec3 a, const Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// @Optimize: simd would be great here
inline Vec3 vec3_lerp(const Vec3 a, const Vec3 b, const f32 t) {
    Vec3 result;
    result.x = lerp(a.x, b.x, t);
    result.y = lerp(a.y, b.y, t);
    result.z = lerp(a.z, b.z, t);
    
    return (result);
}

// =====================================================


void mat4_identity(f32 *result) {
    memset(result, 0, 16*sizeof(f32));
    result[0] = 1.0f;
    result[5] = 1.0f;
    result[10] = 1.0f;
    result[15] = 1.0f;
}

void mat4_perspective(const float fov,const float aspect_ratio,const float near_,const float far_, f32 *result) {
    memset(result, 0, 16*sizeof(f32));
    
    const f32 tan_theta_2 = tan(radians(fov) * 0.5f);
    
    result[0] = 1.0f / (aspect_ratio * tan_theta_2);
    result[5] = -1.0f / tan_theta_2;
    result[10] = far_ / (near_ - far_);
    result[11] = -1.0f;
    result[14] = (near_ * far_) / (near_ - far_);
    result[15] = 0.0f;
}

void mat4_perspective_gl(const f32 fov, const f32 aspect_ratio, const f32 znear, const f32 zfar, f32 *result) {
    memset(result, 0, 16*sizeof(f32));
    const f32 tan_theta_2 = tan(radians(fov) * 0.5f);
    
    result[0] = 1.0f / (aspect_ratio * tan_theta_2);
    result[5] = 1.0f / tan_theta_2;
    result[10] = (znear + zfar) / (znear - zfar);
    result[11] = -1.0f;
    result[14] = (2.0f * znear * zfar) / (znear - zfar);
    result[15] = 0.0f;
}

void mat4_ortho(const float t,const float b,const float l,const float r,const float znear,const float zfar, f32 *result) {
    memset(result, 0, 16*sizeof(f32));
    
    result[0] = 2.0f / (r - l);
    result[5] = 2.0f / (b - t);
    result[10] = -2.0f / (zfar - znear);
    result[12] = -(r + l) / (r - l);
    result[13] = -(b + t) / (b - t);
    result[14] = -znear / (zfar - znear);
    result[15] = 1.0f;
}

void mat4_ortho_gl(const float t,const float b,const float l,const float r,const float znear,const float zfar,f32 *result) {
    memset(result, 0, 16*sizeof(f32));
    
    result[0] = 2.0f / (r - l);
    result[5] = 2.0f / (t - b);
    result[10] = -2.0f / (zfar - znear);
    result[12] = -(r + l) / (r - l);
    result[13] = -(t + b) / (t - b);
    result[14] = -znear / (zfar - znear);
    result[15] = 1.0f;
}

void mat4_ortho_zoom(float ratio, float zoom, float n, float f, f32 *result) {
    float width = ratio * zoom;
    float height = (1.0f / ratio) * zoom;
    float l = -width;
    float r = width;
    float t = height;
    float b = -height;
    
    memset(result, 0, 16*sizeof(f32));
    
    result[0] = 2.0f / (r - l);
    result[5] = 2.0f / (b - t);
    result[10] = -2.0f / (f - n);
    result[12] = -(r + l) / (r - l);
    result[13] = -(b + t) / (b - t);
    result[14] = -n / (f - n);
    result[15] = 1.0f;
    
}

void mat4_ortho_zoom_gl(float ratio, float zoom, float n, float f, f32 *result) {
    float width = ratio * zoom;
    float height = (1.0f / ratio) * zoom;
    float l = -width;
    float r = width;
    float t = height;
    float b = -height;
    
    memset(result, 0, 16*sizeof(f32));
    
    result[0] = 2.0f / (r - l);
    result[5] = 2.0f / (t - b);
    result[10] = -2.0f / (f - n);
    result[12] = -(r + l) / (r - l);
    result[13] = -(t + b) / (t - b);
    result[14] = -(f + n) / (f - n);
    result[15] = 1.0f;
}

void mat4_look_at(const Vec3 target, const Vec3 eye, const Vec3 up, f32 *mat) {
    Vec3 z_axis = vec3_normalize(vec3_sub(eye, target));
    Vec3 x_axis = vec3_normalize(vec3_cross(up, z_axis));
    Vec3 y_axis = vec3_cross(z_axis, x_axis);
    
    mat[0] = x_axis.x;
    mat[1] = y_axis.x;
    mat[2] = z_axis.x;
    mat[3] = 0.0f;
    
    mat[4] = x_axis.y;
    mat[5] = y_axis.y;
    mat[6] = z_axis.y;
    mat[7] = 0.0f;
    
    mat[8] = x_axis.z;
    mat[9] = y_axis.z;
    mat[10] = z_axis.z;
    mat[11] = 0.0f;
    
    mat[12] = -vec3_dot(x_axis, eye);
    mat[13] = -vec3_dot(y_axis, eye);
    mat[14] = -vec3_dot(z_axis, eye);
    mat[15] = 1.0f;
}

inline void trs_quat_to_mat4(const Vec3 *t, const Quat *r, const Vec3 *s, f32 *restrict dst) {
    const f32 sqx = 2.0f * r->x * r->x;
    const f32 sqy = 2.0f * r->y * r->y;
    const f32 sqz = 2.0f * r->z * r->z;
    //const float sqw = 2.0f * r->w * r->w; // @TODO This is unused, error ?
    
    const f32 xy = r->x * r->y;
    const f32 zw = r->z * r->w;
    
    const f32 xz = r->x * r->z;
    const f32 yw = r->y * r->w;
    
    const f32 yz = r->y * r->z;
    const f32 xw = r->x * r->w;
    
    dst[0] = (1 - sqy - sqz) * s->x;
    dst[1] = 2.0f * (xy + zw) * s->x;
    dst[2] = 2.0f * (xz - yw) * s->x;
    dst[3] = 0.0f;
    
    dst[4] = 2.0f * (xy - zw) * s->y;
    dst[5] = (1.0f - sqx - sqz) * s->y;
    dst[6] = 2.0f * (yz + xw) * s->y;
    dst[7] = 0.0f;
    
    dst[8] = 2.0f * (xz + yw) * s->z;
    dst[9] = 2.0f * (yz - xw) * s->z;
    dst[10] = (1.0f - sqx - sqy) * s->z;
    dst[11] = 0.0f;
    
    dst[12] = t->x;
    dst[13] = t->y;
    dst[14] = t->z;
    dst[15] = 1.0f;
}

void trs_to_mat4(const Vec3 t, const Vec3 r, const Vec3 s, f32 *result) {
    mat4_identity(result);
    mat4_translateby(result, t);
    mat4_rotate_euler(result, r);
    mat4_scaleby(result, s);
}

void mat4_transpose(Mat4 *restrict mat) {
    Mat4 tmp = {0};
    
    for(u32 c = 0; c < 4; c++) {
        for(u32 r = 0; r < 4; r++) {
            tmp[c * 4 + r] = *mat[r * 4 + c];
        }
    }
    
    for(u32 i = 0; i < 16; i++) {
        *mat[i] = tmp[i];
    }
}

void mat4_mul(const f32 *restrict const a, const f32 *restrict const b, f32 *result) {
    memset(result, 0, 16*sizeof(f32));
    
    for(u32 i = 0; i < 4; i++) {
        for(u32 j = 0; j < 4; j++) {
            for(u32 k = 0; k < 4; k++) {
                result[j * 4 + i] += (a[k * 4 + i] * b[j * 4 + k]);
            }
        }
    }
}

Vec4 mat4_mul_vec4(const f32 *restrict const mat, const Vec4 vec) {
    Vec4 result = {0};
    result.x = vec.x * mat[0] + vec.y * mat[4] + vec.z * mat[8] + vec.w * mat[12];
    result.y = vec.x * mat[1] + vec.y * mat[5] + vec.z * mat[9] + vec.w * mat[13];
    result.z = vec.x * mat[2] + vec.y * mat[6] + vec.z * mat[10] + vec.w * mat[14];
    result.w = vec.x * mat[3] + vec.y * mat[7] + vec.z * mat[11] + vec.w * mat[15];
    return result;
}

// This assumes that w == 1
Vec3 mat4_mul_vec3(const f32 *const mat, const Vec3 vec) {
    Vec3 result = {0};
    result.x = vec.x * mat[0] + vec.y * mat[4] + vec.z * mat[8] + mat[12];
    result.y = vec.x * mat[1] + vec.y * mat[5] + vec.z * mat[9] + mat[13];
    result.z = vec.x * mat[2] + vec.y * mat[6] + vec.z * mat[10] + mat[14];
    return result;
}

void mat4_inverse(const f32 *const restrict m, f32 *out) {
    Mat4 inv;
    float det;
    int i;
    
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] -
        m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
        m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] +
        m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
        m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] -
        m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
        m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] +
        m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
        m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] +
        m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
        m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] -
        m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
        m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] +
        m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
        m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] -
        m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
        m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] -
        m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
        m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] +
        m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
        m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] -
        m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
        m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] +
        m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
        m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] +
        m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
        m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] -
        m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
        m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] +
        m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
        m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] -
        m[4] * m[1] * m[10] + m[4] * m[2] * m[9] +
        m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
    
    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    
    if(det == 0) {
        sLog("Inverse of matrix doesn't exist");
        return;
    }
    
    det = 1.0 / det;
    
    for(i = 0; i < 16; i++)
        out[i] = inv[i] * det;
}

Vec3 mat4_get_translation(const Mat4 mat) {
    Vec3 result = *(Vec3 *)&mat[12]; // Haxxxxxxxorzzz
    return result;
}

inline void mat4_translateby(f32 *mat, const Vec3 vec) {
    mat[12] += vec.x;
    mat[13] += vec.y;
    mat[14] += vec.z;
}

inline void mat4_set_position(f32 *mat, const Vec3 vec) {
    mat[12] = vec.x;
    mat[13] = vec.y;
    mat[14] = vec.z;
}

void mat4_rotation_x(f32 *mat, const float radians) {
    mat[0] = 1.0f;
    mat[5] = cos(radians);
    mat[6] = -sin(radians);
    mat[9] = sin(radians);
    mat[10] = cos(radians);
    mat[15] = 1.0f;
}

void mat4_rotation_y(f32 *mat, const float radians) {
    mat[0] = cos(radians);
    mat[2] = sin(radians);
    mat[5] = 1.0f;
    mat[8] = -sin(radians);
    mat[10] = cos(radians);
    mat[15] = 1.0f;
}

void mat4_rotation_z(f32 *mat, const float radians) {
    mat[0] = cos(radians);
    mat[1] = -sin(radians);
    mat[4] = sin(radians);
    mat[1*4+1] = cos(radians);
    mat[2*4+2] = 1.0f;
    mat[3*4+3] = 1.0f;
}

void mat4_rotate_euler(f32 *restrict mat, const Vec3 euler) {
    const f32 cx = cos(euler.x);
    const f32 sx = sin(euler.x);
    const f32 cy = cos(euler.y);
    const f32 sy = sin(euler.y);
    const f32 cz = cos(euler.z);
    const f32 sz = sin(euler.z);
    
    const f32 t01 = -sz * cx;
    const f32 t02 = sz * sx;
    const f32 t11 = cz * cx;
    const f32 t12 = cz * -sx;
    
    mat[0] = cz * cy + t02 * -sy;
    mat[1] = t01;
    mat[2] = cz * sy + t02 * cy;
    
    mat[1*4+0] = sz * cy + t12 * -sy;
    mat[1*4+1] = t11;
    mat[1*4+2] = sz * sy + t12 * cy;
    
    mat[2*4+0] = cx * -sy;
    mat[2*4+1] = sx;
    mat[2*4+2] = cx * cy;
}

Vec3 mat4_get_scale(const f32 *mat) {
    // To get the scale we need the length of the vectors of the first three lines.
    // @Optimize
    Vec3 x = *(Vec3 *)&mat[0];
    Vec3 y = *(Vec3 *)&mat[4];
    Vec3 z = *(Vec3 *)&mat[8];
    
    return (Vec3){vec3_length(x), vec3_length(y), vec3_length(z)};
}

void mat4_scaleby(f32 *restrict dst, const Vec3 s) {
    dst[0] *= s.x;
    dst[1] *= s.x;
    dst[2] *= s.x;
    dst[4] *= s.y;
    dst[5] *= s.y;
    dst[6] *= s.y;
    dst[8] *= s.z;
    dst[9] *= s.z;
    dst[10] *= s.z;
}

void mat4_print(const f32 *const mat) {
    sLog("\n%#.3f, %#.3f, %#.3f, %#.3f\n%#.3f, %#.3f, %#.3f, %#.3f\n%#.3f, "
         "%#.3f, %#.3f, %#.3f\n%#.3f, %#.3f, %#.3f, %#.3f",
         mat[0],
         mat[1],
         mat[2],
         mat[3],
         mat[4],
         mat[5],
         mat[6],
         mat[7],
         mat[8],
         mat[9],
         mat[10],
         mat[11],
         mat[12],
         mat[13],
         mat[14],
         mat[15]);
}

// =====================================================

Quat mat4_get_rotation(f32 *mat) {
    
    Mat4 src;
    memcpy(src, mat, sizeof(Mat4));
    /*
    vec3_normalize2(*(Vec3 *)&mat[0], (Vec3 *)&src[0]);
    vec3_normalize2(*(Vec3 *)&mat[4], (Vec3 *)&src[4]);
    vec3_normalize2(*(Vec3 *)&mat[8], (Vec3 *)&src[8]);
    
    // We have normalized the matrix, onward to calculation
*/
    Quat result;
    f32 tr = src[0] + src[5] + src[10];
    if(tr > 0) {
        f32 S = sqrt(tr+1.0f) * 2.0f;
        result.w = 0.25 * S;
        result.x = (src[9] - src[6]) / S;
        result.y = (src[3] - src[8]) / S;
        result.z = (src[4] - src[1]) / S;
    } else if(src[0] > src[5] && src[0] > src[10]) {
        f32 S = sqrt(1.0 + src[0] - src[5] - src[10]) * 2.0f;
        result.w = (src[9] - src[6]) / S;
        result.x = 0.25 * S;
        result.y = (src[4] + src[1]) / S;
        result.z = (src[3] + src[8]) / S;
    } else if(src[5] > src[10]) {
        f32 S = sqrt(1.0f + src[5] - src[0] - src[10]) * 2.0f;
        result.w = (src[3] - src[8]) / S;
        result.x = (src[4] + src[1]) / S;
        result.y = 0.25 * S;
        result.z = (src[9] + src[6]) / S;
    } else {
        f32 S = sqrt(1.0 + src[10] - src[0] - src[5]) * 2.0f;
        result.w = (src[4] + src[1]) / S;
        result.y = (src[9] - src[6]) / S;
        result.x = (src[3] + src[8]) / S;
        result.z = 0.25 * S;
    }
    return result;
}

void quat_to_mat4(f32 *dst, const Quat *q) {
    const f32 sqx = 2.0f * q->x * q->x;
    const f32 sqy = 2.0f * q->y * q->y;
    const f32 sqz = 2.0f * q->z * q->z;
    //const float sqw = 2.0f * q->w * q->w; @TODO This is unused error ?
    
    const f32 xy = q->x * q->y;
    const f32 zw = q->z * q->w;
    
    const f32 xz = q->x * q->z;
    const f32 yw = q->y * q->w;
    
    const f32 yz = q->y * q->z;
    const f32 xw = q->x * q->w;
    
    // const float invs = 1.0f / (sqx + sqy + sqz + sqw);
    
    dst[0] = (1 - sqy - sqz);
    dst[1] = 2.0f * (xy + zw);
    dst[2] = 2.0f * (xz - yw);
    dst[3] = 0.0f;
    
    dst[4] = 2.0f * (xy - zw);
    dst[5] = (1.0f - sqx - sqz);
    dst[6] = 2.0f * (yz - xw);
    dst[7] = 0.0f;
    
    dst[8] = 2.0f * (xz + yw);
    dst[9] = 2.0f * (yz + xw);
    dst[10] = (1 - sqx - sqy);
    dst[11] = 0.0f;
    
    dst[12] = 0.0f;
    dst[13] = 0.0f;
    dst[14] = 0.0f;
    dst[15] = 1.0f;
}

Quat quat_from_axis(const Vec3 axis, const f32 angle) {
    Quat result;
    f32 sine = sin(angle/2.0f);
    result.x = axis.x * sine;
    result.y = axis.y * sine;
    result.z = axis.z * sine;
    result.w = cos(angle / 2.0f);
    return (result);
}

Quat quat_lookat(const Vec3 pos, const Vec3 dest, const Vec3 up) {
    Vec3 look = vec3_normalize(vec3_sub(dest, pos));
    Vec3 forw = vec3_normalize((Vec3){0.0f, 0.0f, 1.0f});
    Vec3 w = vec3_cross(forw, look);
    return quat_normalize((Quat){w.x, w.y, w.z, 1.0f + vec3_dot(look, forw)});
}

Quat quat_identity() {
    return (Quat){0.0f, 0.0f, 0.0f, 1.0f};
}

inline Quat quat_normalize(Quat q) {
    Quat result;
    f32 length = sqrt(q.x * q.x +  q.y * q.y + q.z * q.z + q.w * q.w);
    result.x = q.x / length;
    result.y = q.y / length;
    result.z = q.z / length;
    result.w = q.w / length;
    return (result);
}

Quat quat_nlerp(const Quat a, const Quat b, const f32 t) {
    Quat result;
    result.x = lerp(a.x, b.x, t);
    result.y = lerp(a.y, b.y, t);
    result.z = lerp(a.z, b.z, t);
    result.w = lerp(a.w, b.w, t);
    return (quat_normalize(result));
}

Quat quat_slerp(const Quat a, Quat b, const f32 t) {
    f32 dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    
    if(dot >= 0.999999f) { // This means that the two quaternions are identical. So return either one.
        return a;
    }
    
    if(dot < 0.0f) {
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        b.w = -b.w;
        dot = -dot;
    }
    
    f32 theta = (f32) acos(dot);
    
    /* // Maybe useless ?
        if (theta < 0.0)
            theta = -theta;
        */
    
	f32 st =  sin(theta);
	f32 sut = (f32) sin(t * theta);
	f32 sout = (f32) sin((1-t) * theta);
	f32 k0 = sout/st;
	f32 k1 = sut/st;
    
    Quat result;
    
	result.x = k0 * a.x + k1 * b.x;
	result.y = k0 * a.y + k1 * b.y;
	result.z = k0 * a.z + k1 * b.z;
	result.w = k0 * a.w + k1 * b.w;
    
	return(quat_normalize(result));
}

void quat_sprint(const Quat q, char *buf) {
    sprintf(buf, "x: %.2f y: %.2f z: %.2f w: %.2f", q.x, q.y, q.w, q.w);
}

// --------
// TRANSFORM

inline void transform_identity(Transform *xform) {
    xform->translation = (Vec3){0, 0, 0};
    xform->rotation = (Quat){0, 0, 0, 1};
    xform->scale = (Vec3){1, 1, 1};
}

inline void transform_to_mat4(const Transform *xform, Mat4* mat) {
    trs_quat_to_mat4(&xform->translation, &xform->rotation, &xform->scale, (f32 *)mat);
}

// @Optimize, we do stuff in double when doing this
inline void mat4_to_transform(const Mat4 *mat, Transform *xform) {
    xform->translation = mat4_get_translation((f32 *)mat);
    xform->rotation = mat4_get_rotation((f32 *)mat);
    xform->scale = mat4_get_scale((f32 *)mat);
}

// =================================

inline f32 lerp(f32 a, f32 b, f32 t) {
    return a + (t * (b - a));
}

