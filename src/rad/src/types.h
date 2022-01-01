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

//! Size of a "small" face in units.
constexpr float SMALL_FACE_SIZE = 12.0f;

//! Min patch size if face is too small. The actual size may be down to a half of this value
//! (depends on original size before subdivision).
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
    //! List of faces that are in this plane
    std::vector<unsigned> faces;

    void load(RadSimImpl &radSim, const bsp::BSPPlane &bspPlane);
};

struct Face : public bsp::BSPFace {
    struct Vertex {
        glm::vec3 vWorldPos; //!< Position in world space.
        glm::vec2 vFacePos;  //!< Position in face space.
    };

    //! Plane in which face is located.
    const Plane *pPlane = nullptr;

    //! Flags (see FACE_ constants).
    unsigned iFlags = 0;

    float flPlaneDist;
    glm::vec3 vNormal;                //! World normal vector. +/- pPlane->vNormal
    glm::dvec3 vWorldOrigin;          //!< Origin of face coord system.
    glm::dvec3 vFaceI, vFaceJ;        //!< Face axis in world coords.
    glm::vec2 vFaceCenter;            //!< Center point in face coords
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

    inline glm::vec3 faceToWorld(const glm::vec2 &face) const {
        return rad::faceToWorld(face, vWorldOrigin, vFaceI, vFaceJ);
    }

    inline glm::vec2 worldToFace(const glm::vec3 &world) const {
        return rad::worldToFace(world, vFaceI, vFaceJ);
    }
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
