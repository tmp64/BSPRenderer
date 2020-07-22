#include <bsp/vertices_lump.h>
#include <bsp/level.h>

void bsp::VerticesLump::loadLump(appfw::span<uint8_t> lumpData) {
    if (lumpData.size() % sizeof(glm::vec3) != 0) {
        throw LevelFormatException("Vertices lump is of incorrect size");
    }

    size_t count = lumpData.size() / sizeof(glm::vec3);
    m_Vertices.resize(count);
    memcpy(m_Vertices.data(), lumpData.data(), lumpData.size());
}
