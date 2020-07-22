#include <bsp/leaves_lump.h>
#include <bsp/level.h>

void bsp::LeavesLump::loadLump(appfw::span<uint8_t> lumpData) {
    if (lumpData.size() % sizeof(BSPLeaf) != 0) {
        throw LevelFormatException("Leaves lump is of incorrect size");
    }

    size_t count = lumpData.size() / sizeof(BSPLeaf);
    m_Leaves.resize(count);
    memcpy(m_Leaves.data(), lumpData.data(), lumpData.size());
}
