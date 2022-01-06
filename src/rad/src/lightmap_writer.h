#ifndef RAD_LIGHTMAP_WRITER_H
#define RAD_LIGHTMAP_WRITER_H
#include <appfw/appfw.h>
#include <appfw/binary_file.h>
#include <app_base/texture_block.h>
#include "types.h"

namespace rad {

class RadSimImpl;

class LightmapWriter {
public:
    inline LightmapWriter(RadSimImpl &radSim)
        : m_RadSim(radSim) {}

    void saveLightmap();

private:
    static constexpr float TRACE_OFFSET = 0.1f;
    static constexpr float FILTER_RADIUS = 2.0f;

    struct FaceLightmap {
        glm::ivec2 vSize;
        std::vector<glm::vec2> vTexCoords;
        std::vector<glm::vec3> lightmapData;
        glm::ivec2 vBlockOffset;
        glm::vec2 vFaceOffset; //< Face plane pos of lightmap (0;0)
    };

    RadSimImpl &m_RadSim;
    TextureBlock<glm::vec3> m_TexBlock;

    float m_flLuxelSize = 0;
    int m_iOversampleSize = 0;
    int m_iBlockSize = 0;
    int m_iBlockPadding = 0;

    std::vector<size_t> m_LightmapIdx;
    std::vector<FaceLightmap> m_Lightmaps;

    void processFace(size_t faceIdx);
    void sampleLightmap(FaceLightmap &lm, size_t faceIdx, float luxelSize);
    void sampleFace(const Face &face, glm::vec2 luxelPos, float radius, glm::vec2 filterk,
                    glm::vec3 &out, float &weightSum, bool checkTrace);
    void createBlock();
    void writeLightmapFile();

    static void getCorners(glm::vec2 point, float size, glm::vec2 corners[4]);
    static bool intersectAABB(const glm::vec2 b1[2], const glm::vec2 b2[2]);
};

} // namespace rad

#endif
