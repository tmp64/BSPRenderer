#ifndef RAD_TYPES_H
#define RAD_TYPES_H
#include <bsp/bsp_types.h>
#include "coords.h"

namespace rad {

class RadSimImpl;
class Material;

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

//! Intensity scales to convert linear RGB color into linear intensity
constexpr glm::vec3 RGB_INTENSITY = glm::vec3(0.25f, 0.50f, 0.25f);

/**
 * Epsilon used for floating-point comparisons.
 */
constexpr float EPSILON = 1.f / 64.f;

constexpr float ON_EPSILON = 0.01f;

enum FaceFlags : unsigned {
    FACE_NO_LIGHTMAPS = (1u << 0), //!< Face doesn't need to have lightmaps
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

    //! Material of the face
    Material *pMaterial;

    //! Diffuse light source, in linear space with brightness pre-multiplied.
    glm::vec3 vLightColor;

    //! Length of a side of the square patch.
    float flPatchSize = 0;

    //! Scale of the lightmap, higher number means larger lightmap
    float flLightmapScale = 1;

    //! Base reflectivity, it is multiplied with color value per patch.
    float flBaseReflectivity = 1.0f;

    //! Index of first patch
    PatchIndex iFirstPatch = 0;

    //! Number of patches of this face
    PatchIndex iNumPatches = 0;

    void load(RadSimImpl &radSim, const bsp::BSPFace &bspFace, int faceIndex);

    //! @returns whether the face has lightmaps and should be split into patches
    inline bool hasLightmap() const {
        return !(iFlags & FACE_NO_LIGHTMAPS);
    }

    inline glm::vec3 faceToWorld(const glm::vec2 &face) const {
        return rad::faceToWorld(face, vWorldOrigin, vFaceI, vFaceJ);
    }

    inline glm::vec2 worldToFace(const glm::vec3 &world) const {
        return rad::worldToFace(world, vFaceI, vFaceJ);
    }

    inline int findLightstyle(uint8_t ls) const {
        for (int i = 0; i < bsp::NUM_LIGHTSTYLES; i++) {
            if (nStyles[i] == ls) {
                return i;
            }
        }

        return -1;
    }
};

enum class LightType
{
    Point,
    Spot,
};

//! An entity light.
struct EntLight {
    LightType type = LightType::Point;
    glm::vec3 vOrigin = glm::vec3(0, 0, 0);
    //glm::vec3 vDirection = glm::vec3(0, 0, 0);
    glm::vec3 vLight = glm::vec3(0, 0, 0); //!< Linear color times intensity
    float flLinear = 0.3f;                 //!< Linear term of attenuation
    float flQuadratic = 0.8f;              //!< Quadratic term of attenuation.
    float flFalloff = 1;                   //!< Scale of attenuation function denominator. Closer to 0 means light travels further.

};

//! A group of lights that can be toggled together.
//! Grouped by name or pattern of entity lights.
struct LightStyle {
    std::string name;    //!< Targetname, can be empty
    std::string pattern; //!< Light blinking pattern (or empty if no pattern)
    std::vector<EntLight> entlights;
    std::vector<int> texlights;
    int iBounceCount = 0;

    inline bool hasLights() { return !entlights.empty() || !texlights.empty(); }
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

//! Returns vector that points in the direction given in degrees (or -1/-2 for yaw).
//! Used for sunlight calculation.
inline glm::vec3 getDirectionFromAngles(float pitch, float yaw) {
    glm::vec3 dir;

    if (yaw == -1) {
        // ANGLE_UP
        dir.x = 0;
        dir.y = 0;
        dir.z = 1;
    } else if (yaw == -2) {
        // ANGLE_DOWN
        dir.x = 0;
        dir.y = 0;
        dir.z = -1;
    } else {
        dir.x = cos(glm::radians(yaw));
        dir.y = sin(glm::radians(yaw));
        dir.z = 0;
    }

    dir.x *= cos(glm::radians(pitch));
    dir.y *= cos(glm::radians(pitch));
    dir.z = sin(glm::radians(pitch));

    return glm::normalize(dir);
}

inline bool isNullVector(glm::vec3 v) {
    return v.x == 0 && v.y == 0 && v.z == 0;
}

} // namespace rad

#endif
