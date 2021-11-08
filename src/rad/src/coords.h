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
inline glm::vec2 worldToPlane(const glm::vec3 &world, const glm::vec3 &vI, const glm::vec3 &vJ) {
    return glm::vec2(glm::dot(world, vI), glm::dot(world, vJ));
}

//! Converts plane coords to world coords
inline glm::vec3 planeToWorld(const glm::vec2 &plane, const glm::vec3 &vWorldOrigin,
                              const glm::vec3 &vI, const glm::vec3 &vJ) {
    return vWorldOrigin + plane.x * vI + plane.y * vJ;
}

//! Converts plane coords to face coords.
inline glm::vec2 planeToFace(const glm::vec2 &plane, const glm::vec2 &vI, const glm::vec2 &vJ) {
    return glm::vec2(glm::dot(plane, vI), glm::dot(plane, vJ));
}

//! Converts face coords to plane coords.
inline glm::vec2 faceToPlane(const glm::vec2 &face, const glm::vec2 &vI, const glm::vec2 &vJ) {
    return face.x * vI + face.y * vJ;
}

} // namespace rad

#endif
