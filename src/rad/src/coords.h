#ifndef RAD_COORDS_H
#define RAD_COORDS_H
#include <glm/glm.hpp>

namespace rad {

/*
Coordinate systems documentation
hu - hammer (the level editor) unit, roughly 1 inch

WORLD COORDINATES:
    3D coordiantes in the world space. 1 u = 1 hu

TEXTURE COORDINATES:
    2D coordinates on a BSP face. Have different scale. Used for textures.
    Face coordinates use same axises as texture coords but have different scale.

FACE COORDINATS:
    2D coordinates on a BSP face. 1 u = 1 hu.
    Used for patches (they are axis-aligned squares in this coord system).

    vWorldOrigin:vec3 is the world origin of point (0,0).
    vI:vec3 (vJ:vec3) is a 3D vector of the plane's X (Y) axis in world space.

COORDINATE SYSTEM CONVERSION:
    world -> face: (assuming world point is on the plane)
        x = dot(world, vI);
        y = dot(world, vJ);

    face -> world:
        world = vWorldOrigin + x * vI + y * vJ;
*/

//! Converts world coords to face coords.
inline glm::dvec2 worldToFace(const glm::dvec3 &world, const glm::dvec3 &vI, const glm::dvec3 &vJ) {
    return glm::dvec2(glm::dot(world, vI), glm::dot(world, vJ));
}

//! Converts face coords to world coords
inline glm::dvec3 faceToWorld(const glm::dvec2 &face, const glm::dvec3 &vWorldOrigin,
                              const glm::dvec3 &vI, const glm::dvec3 &vJ) {
    return vWorldOrigin + (double)face.x * vI + (double)face.y * vJ;
}

} // namespace rad

#endif
