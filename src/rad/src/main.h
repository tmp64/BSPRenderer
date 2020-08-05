#ifndef MAIN_H
#define MAIN_H
#include <bsp/level.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>

struct Plane : public bsp::BSPPlane {
    /**
     * Y axis for lightmap and patches.
     * X is calculated as cross(j, n)
     */
    glm::vec3 vJ;
};

enum FaceFlags : unsigned {
    FACE_SKY = (1u << 0),   //!< Face is a sky surface
    FACE_HACK = (1u << 1),  //!< Face is a constant light. TODO: Read lights from a file
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
     * Index of the lightmap in g_Lightmaps.
     */
    int iLightmapIdx = -1;

    /**
     * Size of lightmap in pixels.
     */
    glm::ivec2 lmSize;
};

struct Patch {
    /**
     * Length of a side of the square.
     */
    float flSize = 0;

    /**
     * Center point of the square in world-space.
     */
    glm::vec3 vOrigin;

    /**
     * Normal vector. Points away from front side.
     */
    glm::vec3 vNormal;

    /**
     * Plane in which patch is located.
     */
    //const Plane *pPlane = nullptr;

    /**
     * Final color of the patch.
     */
    glm::vec3 finalColor = {0, 0, 0};

    /**
     * Pointer to lightmap pixel.
     */
    glm::vec3 *pLMPixel = nullptr;

    /**
     * View factors for visible patches.
     */
    std::unordered_map<size_t, float> viewFactors;
};

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
    inline glm::vec3 &getPixel(glm::ivec2 pos) {
        return data[(size_t)pos.y * (size_t)size.x + (size_t)pos.x];
    }
};

struct RADConfig {
    float flPatchSize = 0;
    size_t iBounceCount = 0;
};

extern bsp::Level g_Level;
extern RADConfig g_Config;

extern std::vector<Plane> g_Planes;
extern std::vector<Face> g_Faces;
extern std::vector<Patch> g_Patches;
extern std::vector<LightmapTexture> g_Lightmaps;

#endif
