#include <bsp/level.h>
#include <bsp/planes_lump.h>

void bsp::PlanesLump::loadLump(appfw::span<uint8_t> lumpData) {
    if (lumpData.size() % sizeof(BSPPlane) != 0) {
        throw LevelFormatException("Planes lump is of incorrect size");
    }

    size_t count = lumpData.size() / sizeof(BSPPlane);
    m_Planes.resize(count);
    memcpy(m_Planes.data(), lumpData.data(), lumpData.size());
}
