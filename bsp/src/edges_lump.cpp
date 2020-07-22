#include <bsp/edges_lump.h>
#include <bsp/level.h>

void bsp::EdgesLump::loadLump(appfw::span<uint8_t> lumpData) {
    if (lumpData.size() % sizeof(BSPEdge) != 0) {
        throw LevelFormatException("Edges lump is of incorrect size");
    }

    size_t count = lumpData.size() / sizeof(BSPEdge);
    m_Edges.resize(count);
    memcpy(m_Edges.data(), lumpData.data(), lumpData.size());
}
