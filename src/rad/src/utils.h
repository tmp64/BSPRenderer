#ifndef UTILS_H
#define UTILS_H
#include <bsp/level.h>
#include <vector>

/**
 * Epsilon used fro floating-point comparisons.
 */
constexpr float EPSILON = 1.f / 64.f;

constexpr float ON_EPSILON = 0.01f;

/**
 * Returns true if two floats are almost equal.
 */
inline bool floatEquals(float l, float r) { return std::abs(l - r) <= EPSILON; }

/**
 * Returns a point with coordinates (x, 0, 0) on a plane.
 */
inline glm::vec3 getXPointOnPlane(const bsp::BSPPlane &plane) { return {plane.fDist / plane.vNormal.x, 0.f, 0.f}; }

/**
 * Returns a point with coordinates (0, y, 0) on a plane.
 */
inline glm::vec3 getYPointOnPlane(const bsp::BSPPlane &plane) { return {0.f, plane.fDist / plane.vNormal.y, 0.f}; }

/**
 * Returns a point with coordinates (0, 0, z) on a plane.
 */
inline glm::vec3 getZPointOnPlane(const bsp::BSPPlane &plane) { return {0.f, 0.f, plane.fDist / plane.vNormal.z}; }

/**
 * Returns a projection of a point onto a plane.
 */
inline glm::vec3 projectPointOnPlane(const bsp::BSPPlane &plane, glm::vec3 point) {
    float h = glm::dot(plane.vNormal, point) - plane.fDist;
    return point - plane.vNormal * h;
}

/**
 * Returns a projection of a vector onto a plane. Length is not normalized.
 */
inline glm::vec3 projectVectorOnPlane(const bsp::BSPPlane &plane, glm::vec3 vec) {
    glm::vec3 a = projectPointOnPlane(plane, {0.f, 0.f, 0.f});
    glm::vec3 b = projectPointOnPlane(plane, vec);
    return b - a;
}

/**
 * Returns vertices of a face.
 */
std::vector<glm::vec3> getFaceVerteces(const bsp::Level &level, const bsp::BSPFace &face);

#endif
