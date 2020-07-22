#include <bsp/surfedges_lump.h>
#include <bsp/level.h>

void bsp::SurfEdgesLump::loadLump(appfw::span<uint8_t> lumpData) {
    if (lumpData.size() % sizeof(BSPSurfEdge) != 0) {
        throw LevelFormatException("Surfedges lump is of incorrect size");
    }

    size_t count = lumpData.size() / sizeof(BSPSurfEdge);
    m_SurfEdges.resize(count);
    memcpy(m_SurfEdges.data(), lumpData.data(), lumpData.size());
}
