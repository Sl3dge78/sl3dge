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

typedef union Mat4 {
    f32 v[16];
    f32 m[4][4];
} Mat4;

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

void mat4_print(const Mat4 *const mat) {
    sLog("\n%#.3f, %#.3f, %#.3f, %#.3f\n%#.3f, %#.3f, %#.3f, %#.3f\n%#.3f, "
         "%#.3f, %#.3f, %#.3f\n%#.3f, %#.3f, %#.3f, %#.3f",
         mat->v[0],
         mat->v[1],
         mat->v[2],
         mat->v[3],
         mat->v[4],
         mat->v[5],
         mat->v[6],
         mat->v[7],
         mat->v[8],
         mat->v[9],
         mat->v[10],
         mat->v[11],
         mat->v[12],
         mat->v[13],
         mat->v[14],
         mat->v[15]);
}

void mat4_mul(Mat4 *result, const Mat4 *restrict const a, const Mat4 *restrict const b) {
    *result = (Mat4){0};

    for(u32 i = 0; i < 4; i++) {
        for(u32 j = 0; j < 4; j++) {
            for(u32 k = 0; k < 4; k++) {
                result->m[j][i] += (a->m[k][i] * b->m[j][k]);
            }
        }
    }
}

Mat4 mat4_identity() {
    Mat4 result = {{1.0f,
                    0.0f,
                    0.0f,
                    0.0f,
                    0.0f,
                    1.0f,
                    0.0f,
                    0.0f,
                    0.0f,
                    0.0f,
                    1.0f,
                    0.0f,
                    0.0f,
                    0.0f,
                    0.0f,
                    1.0f}};

    return result;
}

void mat4_transpose(Mat4 *restrict mat) {
    Mat4 tmp = {0};

    for(u32 c = 0; c < 4; c++) {
        for(u32 r = 0; r < 4; r++) {
            tmp.v[c * 4 + r] = mat->v[r * 4 + c];
        }
    }

    for(u32 i = 0; i < 16; i++) {
        mat->v[i] = tmp.v[i];
    }
}

Mat4 mat4_perspective(const float fov,
                      const float aspect_ratio,
                      const float near_,
                      const float far_) {
    Mat4 result = {0};

    const f32 tan_theta_2 = tan(radians(fov) * 0.5f);

    result.m[0][0] = 1.0f / (aspect_ratio * tan_theta_2);
    result.m[1][1] = -1.0f / tan_theta_2;
    result.m[2][2] = far_ / (near_ - far_);
    result.m[2][3] = -1.0f;
    result.m[3][2] = (near_ * far_) / (near_ - far_);
    result.m[3][3] = 0.0f;

    return result;
}

Mat4 mat4_perspective_gl(const f32 fov, const f32 aspect_ratio, const f32 znear, const f32 zfar) {
    Mat4 result = {0};
    const f32 tan_theta_2 = tan(radians(fov) * 0.5f);

    result.m[0][0] = 1.0f / (aspect_ratio * tan_theta_2);
    result.m[1][1] = 1.0f / tan_theta_2;
    result.m[2][2] = (znear + zfar) / (znear - zfar);
    result.m[2][3] = -1.0f;
    result.m[3][2] = (2.0f * znear * zfar) / (znear - zfar);
    result.m[3][3] = 0.0f;

    return result;
}

Mat4 mat4_ortho(const float t,
                const float b,
                const float l,
                const float r,
                const float znear,
                const float zfar) {
    Mat4 result = {0};

    result.m[0][0] = 2.0f / (r - l);
    result.m[1][1] = 2.0f / (b - t);
    result.m[2][2] = -2.0f / (zfar - znear);
    result.m[3][0] = -(r + l) / (r - l);
    result.m[3][1] = -(b + t) / (b - t);
    result.m[3][2] = -znear / (zfar - znear);
    result.m[3][3] = 1.0f;

    return result;
}

const Mat4 mat4_ortho_gl(const float t,
                         const float b,
                         const float l,
                         const float r,
                         const float znear,
                         const float zfar) {
    Mat4 result = {0};

    result.m[0][0] = 2.0f / (r - l);
    result.m[1][1] = 2.0f / (t - b);
    result.m[2][2] = -2.0f / (zfar - znear);
    result.m[3][0] = -(r + l) / (r - l);
    result.m[3][1] = -(t + b) / (t - b);
    result.m[3][2] = -znear / (zfar - znear);
    result.m[3][3] = 1.0f;

    return result;
}

Mat4 mat4_ortho_zoom(float ratio, float zoom, float n, float f) {
    float width = ratio * zoom;
    float height = (1.0f / ratio) * zoom;
    float l = -width;
    float r = width;
    float t = height;
    float b = -height;

    Mat4 result = {0};

    result.m[0][0] = 2.0f / (r - l);
    result.m[1][1] = 2.0f / (b - t);
    result.m[2][2] = -2.0f / (f - n);
    result.m[3][0] = -(r + l) / (r - l);
    result.m[3][1] = -(b + t) / (b - t);
    result.m[3][2] = -n / (f - n);
    result.m[3][3] = 1.0f;

    return result;
}

Mat4 mat4_ortho_zoom_gl(float ratio, float zoom, float n, float f) {
    float width = ratio * zoom;
    float height = (1.0f / ratio) * zoom;
    float l = -width;
    float r = width;
    float t = height;
    float b = -height;

    Mat4 result = {0};

    result.m[0][0] = 2.0f / (r - l);
    result.m[1][1] = 2.0f / (t - b);
    result.m[2][2] = -2.0f / (f - n);
    result.m[3][0] = -(r + l) / (r - l);
    result.m[3][1] = -(t + b) / (t - b);
    result.m[3][2] = -(f + n) / (f - n);
    result.m[3][3] = 1.0f;

    return result;
}

inline void mat4_translateby(Mat4 *mat, const Vec3 vec) {
    mat->m[3][0] += vec.x;
    mat->m[3][1] += vec.y;
    mat->m[3][2] += vec.z;
}

inline void mat4_set_position(Mat4 *mat, const Vec3 vec) {
    mat->m[3][0] = vec.x;
    mat->m[3][1] = vec.y;
    mat->m[3][2] = vec.z;
}

void mat4_rotation_x(Mat4 *mat, const float radians) {
    mat->m[0][0] = 1.0f;
    mat->m[1][1] = cos(radians);
    mat->m[1][2] = -sin(radians);
    mat->m[2][1] = sin(radians);
    mat->m[2][2] = cos(radians);
    mat->m[3][3] = 1.0f;
}

void mat4_rotation_y(Mat4 *mat, const float radians) {
    mat->m[0][0] = cos(radians);
    mat->m[0][2] = sin(radians);
    mat->m[1][1] = 1.0f;
    mat->m[2][0] = -sin(radians);
    mat->m[2][2] = cos(radians);
    mat->m[3][3] = 1.0f;
}

void mat4_rotation_z(Mat4 *mat, const float radians) {
    mat->m[0][0] = cos(radians);
    mat->m[0][1] = -sin(radians);
    mat->m[1][0] = sin(radians);
    mat->m[1][1] = cos(radians);
    mat->m[2][2] = 1.0f;
    mat->m[3][3] = 1.0f;
}

void mat4_rotate_euler(Mat4 *restrict mat, const Vec3 euler) {
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

    mat->m[0][0] = cz * cy + t02 * -sy;
    mat->m[0][1] = t01;
    mat->m[0][2] = cz * sy + t02 * cy;

    mat->m[1][0] = sz * cy + t12 * -sy;
    mat->m[1][1] = t11;
    mat->m[1][2] = sz * sy + t12 * cy;

    mat->m[2][0] = cx * -sy;
    mat->m[2][1] = sx;
    mat->m[2][2] = cx * cy;
}

Mat4 mat4_look_at(const Vec3 target, const Vec3 eye, const Vec3 up) {
    Vec3 z_axis = vec3_normalize(vec3_sub(eye, target));
    Vec3 x_axis = vec3_normalize(vec3_cross(up, z_axis));
    Vec3 y_axis = vec3_cross(z_axis, x_axis);

    Mat4 mat = mat4_identity();

    mat.m[0][0] = x_axis.x;
    mat.m[0][1] = y_axis.x;
    mat.m[0][2] = z_axis.x;
    mat.m[0][3] = 0.0f;

    mat.m[1][0] = x_axis.y;
    mat.m[1][1] = y_axis.y;
    mat.m[1][2] = z_axis.y;
    mat.m[1][3] = 0.0f;

    mat.m[2][0] = x_axis.z;
    mat.m[2][1] = y_axis.z;
    mat.m[2][2] = z_axis.z;
    mat.m[2][3] = 0.0f;

    mat.m[3][0] = -vec3_dot(x_axis, eye);
    mat.m[3][1] = -vec3_dot(y_axis, eye);
    mat.m[3][2] = -vec3_dot(z_axis, eye);
    mat.m[3][3] = 1.0f;

    return mat;
}

void trs_quat_to_mat4(Mat4 *restrict dst, const Vec3 *t, const Quat *r, const Vec3 *s) {
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

void mat4_scaleby(Mat4 *restrict dst, const Vec3 s) {
    dst->v[0] *= s.x;
    dst->v[1] *= s.x;
    dst->v[2] *= s.x;
    dst->v[4] *= s.y;
    dst->v[5] *= s.y;
    dst->v[6] *= s.y;
    dst->v[8] *= s.z;
    dst->v[9] *= s.z;
    dst->v[10] *= s.z;
}

Mat4 trs_to_mat4(const Vec3 t, const Vec3 r, const Vec3 s) {
    Mat4 result = mat4_identity();
    mat4_translateby(&result, t);
    mat4_rotate_euler(&result, r);
    mat4_scaleby(&result, s);
    return result;
}

void mat4_inverse(const Mat4 *const restrict m, Mat4 *out) {
    Mat4 inv;
    float det;
    int i;

    inv.v[0] = m->v[5] * m->v[10] * m->v[15] - m->v[5] * m->v[11] * m->v[14] -
               m->v[9] * m->v[6] * m->v[15] + m->v[9] * m->v[7] * m->v[14] +
               m->v[13] * m->v[6] * m->v[11] - m->v[13] * m->v[7] * m->v[10];

    inv.v[4] = -m->v[4] * m->v[10] * m->v[15] + m->v[4] * m->v[11] * m->v[14] +
               m->v[8] * m->v[6] * m->v[15] - m->v[8] * m->v[7] * m->v[14] -
               m->v[12] * m->v[6] * m->v[11] + m->v[12] * m->v[7] * m->v[10];

    inv.v[8] = m->v[4] * m->v[9] * m->v[15] - m->v[4] * m->v[11] * m->v[13] -
               m->v[8] * m->v[5] * m->v[15] + m->v[8] * m->v[7] * m->v[13] +
               m->v[12] * m->v[5] * m->v[11] - m->v[12] * m->v[7] * m->v[9];

    inv.v[12] = -m->v[4] * m->v[9] * m->v[14] + m->v[4] * m->v[10] * m->v[13] +
                m->v[8] * m->v[5] * m->v[14] - m->v[8] * m->v[6] * m->v[13] -
                m->v[12] * m->v[5] * m->v[10] + m->v[12] * m->v[6] * m->v[9];

    inv.v[1] = -m->v[1] * m->v[10] * m->v[15] + m->v[1] * m->v[11] * m->v[14] +
               m->v[9] * m->v[2] * m->v[15] - m->v[9] * m->v[3] * m->v[14] -
               m->v[13] * m->v[2] * m->v[11] + m->v[13] * m->v[3] * m->v[10];

    inv.v[5] = m->v[0] * m->v[10] * m->v[15] - m->v[0] * m->v[11] * m->v[14] -
               m->v[8] * m->v[2] * m->v[15] + m->v[8] * m->v[3] * m->v[14] +
               m->v[12] * m->v[2] * m->v[11] - m->v[12] * m->v[3] * m->v[10];

    inv.v[9] = -m->v[0] * m->v[9] * m->v[15] + m->v[0] * m->v[11] * m->v[13] +
               m->v[8] * m->v[1] * m->v[15] - m->v[8] * m->v[3] * m->v[13] -
               m->v[12] * m->v[1] * m->v[11] + m->v[12] * m->v[3] * m->v[9];

    inv.v[13] = m->v[0] * m->v[9] * m->v[14] - m->v[0] * m->v[10] * m->v[13] -
                m->v[8] * m->v[1] * m->v[14] + m->v[8] * m->v[2] * m->v[13] +
                m->v[12] * m->v[1] * m->v[10] - m->v[12] * m->v[2] * m->v[9];

    inv.v[2] = m->v[1] * m->v[6] * m->v[15] - m->v[1] * m->v[7] * m->v[14] -
               m->v[5] * m->v[2] * m->v[15] + m->v[5] * m->v[3] * m->v[14] +
               m->v[13] * m->v[2] * m->v[7] - m->v[13] * m->v[3] * m->v[6];

    inv.v[6] = -m->v[0] * m->v[6] * m->v[15] + m->v[0] * m->v[7] * m->v[14] +
               m->v[4] * m->v[2] * m->v[15] - m->v[4] * m->v[3] * m->v[14] -
               m->v[12] * m->v[2] * m->v[7] + m->v[12] * m->v[3] * m->v[6];

    inv.v[10] = m->v[0] * m->v[5] * m->v[15] - m->v[0] * m->v[7] * m->v[13] -
                m->v[4] * m->v[1] * m->v[15] + m->v[4] * m->v[3] * m->v[13] +
                m->v[12] * m->v[1] * m->v[7] - m->v[12] * m->v[3] * m->v[5];

    inv.v[14] = -m->v[0] * m->v[5] * m->v[14] + m->v[0] * m->v[6] * m->v[13] +
                m->v[4] * m->v[1] * m->v[14] - m->v[4] * m->v[2] * m->v[13] -
                m->v[12] * m->v[1] * m->v[6] + m->v[12] * m->v[2] * m->v[5];

    inv.v[3] = -m->v[1] * m->v[6] * m->v[11] + m->v[1] * m->v[7] * m->v[10] +
               m->v[5] * m->v[2] * m->v[11] - m->v[5] * m->v[3] * m->v[10] -
               m->v[9] * m->v[2] * m->v[7] + m->v[9] * m->v[3] * m->v[6];

    inv.v[7] = m->v[0] * m->v[6] * m->v[11] - m->v[0] * m->v[7] * m->v[10] -
               m->v[4] * m->v[2] * m->v[11] + m->v[4] * m->v[3] * m->v[10] +
               m->v[8] * m->v[2] * m->v[7] - m->v[8] * m->v[3] * m->v[6];

    inv.v[11] = -m->v[0] * m->v[5] * m->v[11] + m->v[0] * m->v[7] * m->v[9] +
                m->v[4] * m->v[1] * m->v[11] - m->v[4] * m->v[3] * m->v[9] -
                m->v[8] * m->v[1] * m->v[7] + m->v[8] * m->v[3] * m->v[5];

    inv.v[15] = m->v[0] * m->v[5] * m->v[10] - m->v[0] * m->v[6] * m->v[9] -
                m->v[4] * m->v[1] * m->v[10] + m->v[4] * m->v[2] * m->v[9] +
                m->v[8] * m->v[1] * m->v[6] - m->v[8] * m->v[2] * m->v[5];

    det = m->v[0] * inv.v[0] + m->v[1] * inv.v[4] + m->v[2] * inv.v[8] + m->v[3] * inv.v[12];

    if(det == 0) {
        sLog("Inverse of matrix doesn't exist");
        return;
    }

    det = 1.0 / det;

    for(i = 0; i < 16; i++)
        out->v[i] = inv.v[i] * det;
}

Vec3 mat4_get_translation(const Mat4 *mat) {
    Vec3 result = *(Vec3 *)&mat->v[12]; // Haxxxxxxxorzzz
    return result;
}

Vec3 mat4_get_scale(const Mat4 *mat) {
    // To get the scale we need the length of the vectors of the first three lines.
    // @Optimize
    Vec3 x = *(Vec3 *)&mat->v[0];
    Vec3 y = *(Vec3 *)&mat->v[4];
    Vec3 z = *(Vec3 *)&mat->v[8];

    return (Vec3){vec3_length(x), vec3_length(y), vec3_length(z)};
}

Vec4 mat4_mul_vec4(const Mat4 *restrict const mat, const Vec4 vec) {
    Vec4 result = {0};
    result.x = vec.x * mat->v[0] + vec.y * mat->v[4] + vec.z * mat->v[8] + vec.w * mat->v[12];
    result.y = vec.x * mat->v[1] + vec.y * mat->v[5] + vec.z * mat->v[9] + vec.w * mat->v[13];
    result.z = vec.x * mat->v[2] + vec.y * mat->v[6] + vec.z * mat->v[10] + vec.w * mat->v[14];
    result.w = vec.x * mat->v[3] + vec.y * mat->v[7] + vec.z * mat->v[11] + vec.w * mat->v[15];
    return result;
}

// This assumes that w == 1
Vec3 mat4_mul_vec3(const Mat4 *const mat, const Vec3 vec) {
    Vec3 result = {0};
    result.x = vec.x * mat->v[0] + vec.y * mat->v[4] + vec.z * mat->v[8] + mat->v[12];
    result.y = vec.x * mat->v[1] + vec.y * mat->v[5] + vec.z * mat->v[9] + mat->v[13];
    result.z = vec.x * mat->v[2] + vec.y * mat->v[6] + vec.z * mat->v[10] + mat->v[14];
    return result;
}

// =====================================================

void quat_to_mat4(Mat4 *dst, const Quat *q) {
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
