#ifndef RAD_TYPES_H
#define RAD_TYPES_H
#include <bsp/bsp_types.h>

namespace rad {

using PatchIndex = uint32_t;
using ViewFactor = std::tuple<PatchIndex, float>;

/**
 * Maximum number of patches.
 * Maximum patch index is (MAX_PATCH_COUNT - 1).
 */
constexpr PatchIndex MAX_PATCH_COUNT = std::numeric_limits<PatchIndex>::max();

/**
 * Epsilon used for floating-point comparisons.
 */
constexpr float EPSILON = 1.f / 64.f;

constexpr float ON_EPSILON = 0.01f;

enum FaceFlags : unsigned {
    FACE_SKY = (1u << 0), //!< Face is a sky surface
};

struct Plane : public bsp::BSPPlane {
    /**
     * Y axis for lightmap and patches.
     * X is calculated as cross(j, n)
     */
    glm::vec3 vJ;
};

struct Face : public bsp::BSPFace {
    struct Vertex {
        /**
         * Position of vertex in world space.
         */
        glm::vec3 vWorldPos;

        /**
         * Normalized position of vertex in its plane (based on vI and vJ).
         * Normalized means that there are vertices with X or Y = 0.
         */
        glm::vec2 vPlanePos;

        /**
         * Lightmap texture coordinate.
         */
        glm::vec2 vLMCoord;
    };

    /**
     * Plane in which face is located.
     */
    const Plane *pPlane = nullptr;

    /**
     * Flags (see FACE_ constants).
     */
    unsigned iFlags = 0;

    /**
     * Normal vector. +/- pPlane->vNormal
     */
    glm::vec3 vNormal;

    /**
     * X axis for lightmap and patches.
     */
    glm::vec3 vI;

    /**
     * Y axis for lightmap and patches.
     */
    glm::vec3 vJ;

    /**
     * World position of (0, 0) plane coord.
     */
    glm::vec3 vPlaneOrigin;

    /**
     * Bounds of the face in plane coords.
     * Should be both zero.
     */
    glm::vec2 planeMinBounds, planeMaxBounds;

    /**
     * List of vertices.
     */
    std::vector<Vertex> vertices;

    /**
     * Length of a side of the square patch.
     */
    float flPatchSize = 0;

    /**
     * Index of first patch
     */
    PatchIndex iFirstPatch = 0;

    /**
     * Number of patches of this face
     */
    PatchIndex iNumPatches = 0;

    /**
     * Index of the lightmap in g_Lightmaps.
     */
    int iLightmapIdx = -1;

    /**
     * Size of lightmap in pixels.
     */
    glm::ivec2 lmSize;
};

/**
 * Sunlight information.
 */
struct EnvLight {
    glm::vec3 vColor = glm::vec3(0, 0, 0);     // color * brightness
    glm::vec3 vDirection = glm::vec3(0, 0, 0); // Direction of sun rays
};

/**
 * Lightmap of a face.
 */
struct LightmapTexture {
    LightmapTexture() = default;

    LightmapTexture(glm::ivec2 s) {
        size = s;
        data.resize((size_t)s.x * s.y);
    }

    /**
     * Size of the texture in pixels.
     */
    glm::ivec2 size;

    /**
     * Data of the lightmap image.
     */
    std::vector<glm::vec3> data;

    /**
     * Returns a pixel of the image.
     */
    inline glm::vec3 &getPixel(glm::ivec2 pos) { return data[(size_t)pos.y * (size_t)size.x + (size_t)pos.x]; }
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