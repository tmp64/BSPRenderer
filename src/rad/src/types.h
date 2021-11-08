#ifndef RAD_TYPES_H
#define RAD_TYPES_H
#include <bsp/bsp_types.h>
#include "coords.h"

namespace rad {

class RadSimImpl;

using PatchIndex = uint32_t;
static_assert(std::atomic<PatchIndex>::is_always_lock_free, "PatchIndex atomic is not lock free");

/**
 * Maximum number of patches.
 * Maximum patch index is (MAX_PATCH_COUNT - 1).
 */
constexpr PatchIndex MAX_PATCH_COUNT = std::numeric_limits<PatchIndex>::max();

//! Min patch size if face is too small (less than a base patch in X or Y)
constexpr float MIN_PATCH_SIZE_FOR_SMALL_FACES = 0.51f;

/**
 * Epsilon used for floating-point comparisons.
 */
constexpr float EPSILON = 1.f / 64.f;

constexpr float ON_EPSILON = 0.01f;

enum FaceFlags : unsigned {
    FACE_SKY = (1u << 0), //!< Face is a sky surface
};

struct Plane : public bsp::BSPPlane {
    glm::vec3 vWorldOrigin;
    glm::vec3 vI, vJ;

    //! List of faces that are in this plane
    std::vector<unsigned> faces;

    void load(RadSimImpl &radSim, const bsp::BSPPlane &bspPlane);
};

struct Face : public bsp::BSPFace {
    struct Vertex {
        glm::vec3 vWorldPos; //!< Position in world space.
        glm::vec2 vPlanePos; //!< Position in plane space.
        glm::vec2 vFacePos;  //!< Position in face space.
    };

    //! Plane in which face is located.
    const Plane *pPlane = nullptr;

    //! Flags (see FACE_ constants).
    unsigned iFlags = 0;

    glm::vec3 vNormal;                //! World normal vector. +/- pPlane->vNormal
    glm::vec3 vWorldOrigin;           //!< Origin of plane/face coord system.
    glm::vec3 vPlaneI, vPlaneJ;       //!< Plane X/Y in world coords.
    glm::vec2 vFaceI, vFaceJ;         //!< Face axis in plane coords.
    glm::vec2 vPlaneMins, vPlaneMaxs; //!< Bounds in the plane coords.
    glm::vec2 vPlaneCenter;           //!< Center point in plane coords
    glm::vec2 vFaceMins, vFaceMaxs;   //!< Bounds in the face coords.

    //! List of vertices.
    std::vector<Vertex> vertices;

    //! Length of a side of the square patch.
    float flPatchSize = 0;

    //! Index of first patch
    PatchIndex iFirstPatch = 0;

    //! Number of patches of this face
    PatchIndex iNumPatches = 0;

    void load(RadSimImpl &radSim, const bsp::BSPFace &bspFace);

    //! @returns whether the face has lightmaps and should be split into patches
    inline bool hasLightmap() const {
        // Skies don't have lightmaps
        return !(iFlags & FACE_SKY);
    }

    inline glm::vec3 faceToWorld(glm::vec2 c) const {
        return rad::planeToWorld(rad::faceToPlane(c, vFaceI, vFaceJ), vWorldOrigin, vPlaneI,
                                 vPlaneJ);
    }

    inline glm::vec2 planeToFace(glm::vec2 c) const { return rad::planeToFace(c, vFaceI, vFaceJ); }
    inline glm::vec2 faceToPlane(glm::vec2 c) const { return rad::faceToPlane(c, vFaceI, vFaceJ); }
};

/**
 * Returns true if two floats are almost equal.
 */
inline bool floatEquals(float l, float r) { return std::abs(l - r) <= EPSILON; }

/**
 * Converts float to int rounding up while checking with EPSILON.
 * Value is >= 1.
 */
inline int texFloatToInt(float flVal) {
    if (flVal < 1 || floatEquals(flVal, 1)) {
        return 1;
    }

    float intpart;
    float fracpart = std::modf(flVal, &intpart);

    if (floatEquals(fracpart, 0)) {
        return (int)intpart;
    }

    return (int)intpart + 1;
}

} // namespace rad

#endif
