#ifndef RAD_COORDS_H
#define RAD_COORDS_H
#include <glm/glm.hpp>

namespace rad {

/*
Coordinate systems documentation
hu - hammer (the level editor) unit, roughly 1 inch

WORLD COORDINATES:
    3D coordiantes in the world space. 1 u = 1 hu

PLANE COORDINATES:
    2D coordinates on a BSP plane. 1 u = 1 hu.
    Used when sampling colors between faces.

    vWorldOrigin:vec3 is the world origin of point (0,0).
    vI:vec3 (vJ:vec3) is a 3D vector of the plane's X (Y) axis in world space.

TEXTURE COORDINATES:
    2D coordinates on a BSP face. Have different scale. Used for textures.
    Face coordinates use same axises as texture coords but have different scale.

FACE COORDINATS:
    2D coordinates on a BSP face. 1 u = 1 hu.
    Used for patches (they are axis-aligned squares in this coord system).

    vI:vec2 (vJ:vec2) is a 2D vector of the face's X (Y) axis in plane space.

COORDINATE SYSTEM CONVERSION:
    world -> plane: (assuming world point is on the plane)
        x = dot(world, vI);
        y = dot(world, vJ);

    plane -> world:
        world = vWorldOrigin + x * vI + y * vJ;

    plane -> face:
        x = dot(plane, vI);
        y = dot(plane, vJ);

    face -> plane:
        plane = x * vI + y * vJ;
*/

//! Converts world coords to plane coords.
inline glm::dvec2 worldToPlane(const glm::dvec3 &world, const glm::dvec3 &vI,
                               const glm::dvec3 &vJ) {
    return glm::dvec2(glm::dot(world, vI), glm::dot(world, vJ));
}

//! Converts plane coords to world coords
inline glm::dvec3 planeToWorld(const glm::dvec2 &plane, const glm::dvec3 &vWorldOrigin,
                              const glm::dvec3 &vI, const glm::dvec3 &vJ) {
    return vWorldOrigin + (double)plane.x * vI + (double)plane.y * vJ;
}

//! Converts plane coords to face coords.
inline glm::dvec2 planeToFace(const glm::dvec2 &plane, const glm::dvec2 &vI, const glm::dvec2 &vJ) {
    return glm::dvec2(glm::dot(plane, vI), glm::dot(plane, vJ));
}

//! Converts face coords to plane coords.
inline glm::dvec2 faceToPlane(const glm::dvec2 &face, const glm::dvec2 &vI, const glm::dvec2 &vJ) {
    return (double)face.x * vI + (double)face.y * vJ;
}

} // namespace rad

#endif
