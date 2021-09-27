#pragma once

#include <math.h>
#include <stdalign.h>

#include "sTypes.h"
#include "sLogging.h"

#define PI 3.1415926535897932384626433f
#define MAX(x, y) x > y ? x : y

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

typedef f32 Mat4[16];

/*
typedef union Mat4 {
    f32 v[16];
    f32 m[4][4];
} Mat4;
*/

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

Vec3 vec3_cross(const Vec3 a, const Vec3 b) {
    Vec3 result;
    
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    
    return result;
}

float vec3_dot(const Vec3 a, const Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// =====================================================

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

void mat4_mul(f32 *result, const f32 *restrict const a, const f32 *restrict const b) {
    memset(result, 0, 16*sizeof(f32));
    
    for(u32 i = 0; i < 4; i++) {
        for(u32 j = 0; j < 4; j++) {
            for(u32 k = 0; k < 4; k++) {
                result[j * 4 + i] += (a[k * 4 + i] * b[j * 4 + k]);
            }
        }
    }
}

void mat4_identity(f32 *result) {
    memset(result, 0, 16*sizeof(f32));
    result[0] = 1.0f;
    result[5] = 1.0f;
    result[10] = 1.0f;
    result[15] = 1.0f;
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

void mat4_perspective(const float fov,
                      const float aspect_ratio,
                      const float near_,
                      const float far_, f32 *result) {
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

void mat4_ortho(const float t,
                const float b,
                const float l,
                const float r,
                const float znear,
                const float zfar, f32 *result) {
    memset(result, 0, 16*sizeof(f32));
    
    result[0] = 2.0f / (r - l);
    result[5] = 2.0f / (b - t);
    result[10] = -2.0f / (zfar - znear);
    result[12] = -(r + l) / (r - l);
    result[13] = -(b + t) / (b - t);
    result[14] = -znear / (zfar - znear);
    result[15] = 1.0f;
}

void mat4_ortho_gl(const float t,
                   const float b,
                   const float l,
                   const float r,
                   const float znear,
                   const float zfar, f32 *result) {
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
    const float cx = cos(euler.x);
    const float sx = sin(euler.x);
    const float cy = cos(euler.y);
    const float sy = sin(euler.y);
    const float cz = cos(euler.z);
    const float sz = sin(euler.z);
    
    const float t01 = -sz * cx;
    const float t02 = sz * sx;
    const float t11 = cz * cx;
    const float t12 = cz * -sx;
    
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

void trs_quat_to_mat4(f32 *restrict dst, const Vec3 *t, const Quat *r, const Vec3 *s) {
    const float sqx = 2.0f * r->x * r->x;
    const float sqy = 2.0f * r->y * r->y;
    const float sqz = 2.0f * r->z * r->z;
    //const float sqw = 2.0f * r->w * r->w; // @TODO This is unused, error ?
    
    const float xy = r->x * r->y;
    const float zw = r->z * r->w;
    
    const float xz = r->x * r->z;
    const float yw = r->y * r->w;
    
    const float yz = r->y * r->z;
    const float xw = r->x * r->w;
    
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

void trs_to_mat4(const Vec3 t, const Vec3 r, const Vec3 s, f32 *result) {
    mat4_identity(result);
    mat4_translateby(result, t);
    mat4_rotate_euler(result, r);
    mat4_scaleby(result, s);
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

Vec3 mat4_get_scale(const Mat4 *mat) {
    // To get the scale we need the length of the vectors of the first three lines.
    // @Optimize
    Vec3 x = *(Vec3 *)&mat[0];
    Vec3 y = *(Vec3 *)&mat[4];
    Vec3 z = *(Vec3 *)&mat[8];
    
    return (Vec3){vec3_length(x), vec3_length(y), vec3_length(z)};
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

// =====================================================

void quat_to_mat4(f32 *dst, const Quat *q) {
    const float sqx = 2.0f * q->x * q->x;
    const float sqy = 2.0f * q->y * q->y;
    const float sqz = 2.0f * q->z * q->z;
    //const float sqw = 2.0f * q->w * q->w; @TODO This is unused error ?
    
    const float xy = q->x * q->y;
    const float zw = q->z * q->w;
    
    const float xz = q->x * q->z;
    const float yw = q->y * q->w;
    
    const float yz = q->y * q->z;
    const float xw = q->x * q->w;
    
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
