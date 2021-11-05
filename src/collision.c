#pragma once

#include "utils/sl3dge.h"

// p : World position of the test
// m : Matrix defining the bounding box
// Returns : true if the point is in the bounding box
bool IsPointInBoundingBox(const Vec3 p, const Mat4 m) {
    // We'll first transform the point by the inverse bb matrix to do our calculations in normalized space
    Mat4 inv;
    mat4_inverse(m, inv);
    
    Vec3 t = mat4_mul_vec3(inv, p);
    
    // The bounding box has sides of size 1 centered on 0, so each side goes from -0.5 to 0.5 except Z
    if(t.x < -0.5f || t.x > 0.5f ||
       t.y < 0.0f || t.y > 1.0f ||
       t.z < -0.5f || t.z > 0.5f)
        return false;
    else
        return true;
}

// plane_normal is the plane's position and normal. Works great for AABBs
bool IsLineIntersectingAAPlane(const Vec3 l1, const Vec3 l2, const Vec3 plane_normal) {
    f32 dot1 = vec3_dot(vec3_sub(l1, plane_normal), plane_normal);
    f32 dot2 = vec3_dot(vec3_sub(l2, plane_normal), plane_normal);
    return dot1 * dot2 <= 0;
}

// plane_pos: the plane's position in space
// normal: the plane's normal
bool IsLineIntersectingPlane(const Vec3 l1, const Vec3 l2, const Vec3 plane_pos, const Vec3 normal) {
    f32 dot1 = vec3_dot(vec3_sub(l1, plane_pos), normal);
    f32 dot2 = vec3_dot(vec3_sub(l2, plane_pos), normal);
    return dot1 * dot2 <= 0;
}

bool IsLineIntersectingBoundingBox(const Vec3 l1, const Vec3 l2, const Transform *xform) {
    // Transform the line into the bb coord system
    Mat4 m;
    transform_to_mat4(xform, &m);
    
    Mat4 inv;
    mat4_inverse(m, inv);
    
    Vec3 lb1 = mat4_mul_vec3(inv, l1);
    Vec3 lb2 = mat4_mul_vec3(inv, l2);
    
    Vec3 r_org = lb1;
    Vec3 r_dir = vec3_sub(lb2, lb1);
    f32 length = vec3_length(r_dir);
    r_dir = (Vec3){r_dir.x / length, r_dir.y / length, r_dir.z / length};
    
    Vec3 dirfrac;
    dirfrac.x = 1.0f / r_dir.x;
    dirfrac.y = 1.0f / r_dir.y;
    dirfrac.z = 1.0f / r_dir.z;
    
    f32 t1 = (-0.5f - r_org.x) * dirfrac.x;
    f32 t2 = (0.5f - r_org.x) * dirfrac.x;
    f32 t3 = (0.0f - r_org.y) * dirfrac.y;
    f32 t4 = (1.0f - r_org.y) * dirfrac.y;
    f32 t5 = (-0.5f - r_org.z) * dirfrac.z;
    f32 t6 = (0.5f - r_org.z) * dirfrac.z;
    f32 tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    f32 tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));
    
    // if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
    f32 t = 0.0f;
    if(tmax < 0) {
        t = tmax;
        return false;
    }
    
    // if tmin > tmax, ray doesn't intersect AABB
    if(tmin > tmax) {
        t = tmax;
        return false;
    }
    
    t = tmin;
    return t < length;
}

#ifdef TESTING
void TESTCOLLISION() {
    sLog("Collision");
    
    Mat4 mat = mat4_identity();
    mat4_scaleby(&mat, (Vec3){2.3f, 5.6f, 0.4f});
    mat4_translateby(&mat, (Vec3){1.0f, 4.0f, -5.0f});
    
    TEST_BOOL(IsPointInBoundingBox((Vec3){0.0f, 5.0f, -5.0f}, &mat));
    TEST_BOOL(IsPointInBoundingBox((Vec3){1.0f, 4.1f, -5.0f}, &mat));
    TEST_BOOL(!IsPointInBoundingBox((Vec3){-1.0f, 0.0f, -5.0f}, &mat));
    TEST_BOOL(!IsPointInBoundingBox((Vec3){0.0f, 0.0f, -0.0f}, &mat));
    
    TEST_BOOL(IsLineIntersectingAAPlane((Vec3){-1.0f, 0.0f, 0.0f}, (Vec3){1.0f, 0.0f, 0.0f}, (Vec3){0.5f, 0.0f, 0.0f}));
    TEST_BOOL(!IsLineIntersectingAAPlane((Vec3){-1.0f, 0.5f, 0.0f}, (Vec3){1.0f, 0.5f, 0.0f}, (Vec3){0.0f, 1.0f, 0.0f}));
    
    sLog("Cube");
    mat = mat4_identity();
    
    // Crossing 2 parallel planes
    Vec3 l1 = {1.0f, 0.1f, 0.0f};
    Vec3 l2 = {-1.0f, 0.1f, 0.0f};
    TEST_BOOL(IsLineIntersectingBoundingBox(l1, l2, &mat));
    
    // From inside
    l1 = (Vec3){0.0f, 0.5f, 0.0f};
    l2 = (Vec3){-1.0f, 0.0f, 0.0f};
    TEST_BOOL(IsLineIntersectingBoundingBox(l1, l2, &mat));
    
    // From outside
    l1 = (Vec3){-1.0f, 0.5f, 0.0f};
    l2 = (Vec3){0.0f, 0.5f, 0.0f};
    TEST_BOOL(IsLineIntersectingBoundingBox(l1, l2, &mat));
    
    // Miss Parallel
    l1 = (Vec3){-1.0f, 1.0f, 0.0f};
    l2 = (Vec3){-1.0f, 0.0f, 0.0f};
    TEST_BOOL(!IsLineIntersectingBoundingBox(l1, l2, &mat));
    
    // Hit xz planes
    l1 = (Vec3){-1.0f, 0.5f, 0.0f};
    l2 = (Vec3){0.0f, 0.5f, -1.0f};
    TEST_BOOL(IsLineIntersectingBoundingBox(l1, l2, &mat));
    
    // Hit zy planes
    l1 = (Vec3){0.0f, -0.5f, 0.0f};
    l2 = (Vec3){0.0f, 0.5f, -1.0f};
    TEST_BOOL(IsLineIntersectingBoundingBox(l1, l2, &mat));
    
    // Too short
    l1 = (Vec3){1.0f, 0.5f, 0.0f};
    l2 = (Vec3){0.5f, 0.5f, 0.0f};
    TEST_BOOL(!IsLineIntersectingBoundingBox(l1, l2, &mat));
    
    // Matrix rotation
    l1 = (Vec3){0.0f, -0.5f, 0.0f};
    l2 = (Vec3){0.0f, 0.5f, -1.0f};
    mat4_rotate_euler(&mat, (Vec3){45.0f, 45.0f, 45.0f});
    TEST_BOOL(!IsLineIntersectingBoundingBox(l1, l2, &mat));
    
    sLog("");
}
#endif