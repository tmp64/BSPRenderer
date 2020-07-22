#include <bsp/nodes_lump.h>
#include <bsp/level.h>

void bsp::NodesLump::loadLump(appfw::span<uint8_t> lumpData) {
    if (lumpData.size() % sizeof(BSPNode) != 0) {
        throw LevelFormatException("Nodes lump is of incorrect size");
    }

    size_t count = lumpData.size() / sizeof(BSPNode);
    m_Nodes.resize(count);
    memcpy(m_Nodes.data(), lumpData.data(), lumpData.size());
}
