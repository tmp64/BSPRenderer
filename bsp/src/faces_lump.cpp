#include <bsp/level.h>
#include <bsp/faces_lump.h>

void bsp::FacesLump::loadLump(appfw::span<uint8_t> lumpData) {
    if (lumpData.size() % sizeof(BSPFace) != 0) {
        throw LevelFormatException("Faces lump is of incorrect size");
    }

    size_t count = lumpData.size() / sizeof(BSPFace);
    m_Faces.resize(count);
    memcpy(m_Faces.data(), lumpData.data(), lumpData.size());
}
