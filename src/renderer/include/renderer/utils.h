#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H
#include <cmath>
#include <glm/glm.hpp>
#include <bsp/bsp_types.h>
#include <appfw/utils.h>

enum { ANGLE_PITCH = 0, ANGLE_YAW = 1, ANGLE_ROLL = 2 };

inline void angleVectors(glm::vec3 angles, glm::vec3 *forward, glm::vec3 *right, glm::vec3 *up) {
    float sr, sp, sy, cr, cp, cy;

    sy = glm::sin(glm::radians(angles[ANGLE_YAW]));
    cy = glm::cos(glm::radians(angles[ANGLE_YAW]));

    sp = glm::sin(glm::radians(angles[ANGLE_PITCH]));
    cp = glm::cos(glm::radians(angles[ANGLE_PITCH]));

    sr = glm::sin(glm::radians(angles[ANGLE_ROLL]));
    cr = glm::cos(glm::radians(angles[ANGLE_ROLL]));

    if (forward) {
        forward->x = cp * cy;
        forward->y = cp * sy;
        forward->z = -sp;
    }

    if (right) {
        right->x = (-1.0f * sr * sp * cy + -1.0f * cr * -sy);
        right->y = (-1.0f * sr * sp * sy + -1.0f * cr * cy);
        right->z = (-1.0f * sr * cp);
    }

    if (up) {
        up->x = (cr * sp * cy + -sr * -sy);
        up->y = (cr * sp * sy + -sr * cy);
        up->z = (cr * cp);
    }
}

inline glm::vec3 vectorMA(glm::vec3 start, float scale, glm::vec3 direction) {
    glm::vec3 dest;
    dest.x = start.x + direction.x * scale;
    dest.y = start.y + direction.y * scale;
    dest.z = start.z + direction.z * scale;
    return dest;
}

inline void vectorVectors(glm::vec3 forward, glm::vec3 &right, glm::vec3 &up) {
    float d;

    right[0] = forward[2];
    right[1] = -forward[0];
    right[2] = forward[1];

    d = glm::dot(forward, right);
    right = vectorMA(right, -d, forward);
    right = glm::normalize(right);
    up = glm::cross(right, forward);
}

inline glm::vec3 rotatePointAroundVector(glm::vec3 dir, glm::vec3 point, float degrees) {
    float t0, t1;
    float angle, c, s;
    glm::vec3 vr, vu, vf;
    glm::vec3 dst;

    angle = glm::radians(degrees);
    s = glm::sin(angle);
    c = glm::cos(angle);
    vf = dir;
    vectorVectors(vf, vr, vu);

    t0 = vr[0] * c + vu[0] * -s;
    t1 = vr[0] * s + vu[0] * c;
    dst[0] = (t0 * vr[0] + t1 * vu[0] + vf[0] * vf[0]) * point[0] +
             (t0 * vr[1] + t1 * vu[1] + vf[0] * vf[1]) * point[1] +
             (t0 * vr[2] + t1 * vu[2] + vf[0] * vf[2]) * point[2];

    t0 = vr[1] * c + vu[1] * -s;
    t1 = vr[1] * s + vu[1] * c;
    dst[1] = (t0 * vr[0] + t1 * vu[0] + vf[1] * vf[0]) * point[0] +
             (t0 * vr[1] + t1 * vu[1] + vf[1] * vf[1]) * point[1] +
             (t0 * vr[2] + t1 * vu[2] + vf[1] * vf[2]) * point[2];

    t0 = vr[2] * c + vu[2] * -s;
    t1 = vr[2] * s + vu[2] * c;
    dst[2] = (t0 * vr[0] + t1 * vu[0] + vf[2] * vf[0]) * point[0] +
             (t0 * vr[1] + t1 * vu[1] + vf[2] * vf[1]) * point[1] +
             (t0 * vr[2] + t1 * vu[2] + vf[2] * vf[2]) * point[2];

    return dst;
}

/**
 * Fast box on planeside test
 */
inline uint8_t signbitsForPlane(glm::vec3 normal) {
    uint8_t bits = 0;

    for (int i = 0; i < 3; i++)
        if (normal[i] < 0.0f)
            bits |= 1 << i;
    return bits;
}

/*
================
Matrix4x4_CreateProjection

NOTE: produce quake style world orientation
================
*/
inline glm::mat4 Matrix4x4_CreateProjection(float xMax, float xMin, float yMax, float yMin, float zNear,
                                float zFar) {
    glm::mat4 out;

    out[0][0] = (2.0f * zNear) / (xMax - xMin);
    out[1][1] = (2.0f * zNear) / (yMax - yMin);
    out[2][2] = -(zFar + zNear) / (zFar - zNear);
    out[3][3] = out[0][1] = out[1][0] = out[3][0] = out[0][3] = out[3][1] = out[1][3] = 0.0f;

    out[2][0] = 0.0f;
    out[2][1] = 0.0f;
    out[0][2] = (xMax + xMin) / (xMax - xMin);
    out[1][2] = (yMax + yMin) / (yMax - yMin);
    out[3][2] = -1.0f;
    out[2][3] = -(2.0f * zFar * zNear) / (zFar - zNear);

    return out;
}

inline float planeDiff(glm::vec3 point, const bsp::BSPPlane &plane) {
    float res = 0;

    switch (plane.nType) {
    case bsp::PlaneType::PlaneX:
        res = point[0];
        break;
    case bsp::PlaneType::PlaneY:
        res = point[1];
        break;
    case bsp::PlaneType::PlaneZ:
        res = point[2];
        break;
    default:
        res = glm::dot(point, plane.vNormal);
        break;
    }

    return res - plane.fDist;
}

#endif
