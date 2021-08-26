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
    static constexpr float FILTER_RADIUS = 2.0f;

    struct FaceLightmap {
        glm::ivec2 vSize;
        std::vector<glm::vec2> vTexCoords;
        std::vector<glm::vec3> lightmapData;
        glm::ivec2 vBlockOffset;
        glm::vec2 vPlaneOffset; //< Face plane pos of lightmap (0;0)
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
    void sampleLightmap(FaceLightmap &lm, size_t faceIdx);
    void createBlock();
    void writeLightmapFile();
};

} // namespace rad

#endif
