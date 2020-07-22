#include <fstream>
#include <vector>
#include <bsp/level.h>
#include <fmt/format.h>

using namespace std::literals::string_literals;

bsp::Level::Level(appfw::span<uint8_t> data) { loadFromBytes(data); }

bsp::Level::Level(const std::string &filename) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (file.fail()) {
        throw std::runtime_error(std::string("Failed to open file: ") + strerror(errno));
    }

	std::vector<uint8_t> data;
    data.reserve(4 * 1024 * 1024);  // Set capacity to 4 MB

    for (;;) {
        uint8_t c;
        file.read((char *)&c, 1);
        if (file.eof()) {
            break;
        }
        data.push_back(c);
    }

    file.close();
    data.shrink_to_fit();

    loadFromBytes(data);
}

void bsp::Level::loadFromBytes(appfw::span<uint8_t> data) {
    if (data.empty()) {
        throw LevelFormatException("File is empty");
    }

    // Read BSP header
    BSPHeader bspHeader;
    
    if (data.size() < sizeof(BSPHeader)) {
        throw LevelFormatException("File is too small for header");
    }

    memcpy(&bspHeader, data.data(), sizeof(bspHeader));

    if (bspHeader.nVersion != HL_BSP_VERSION) {
        throw LevelFormatException(fmt::format("Version is invalid. Expected {}, got {}", HL_BSP_VERSION, bspHeader.nVersion));
    }

    // Load lumps
    auto fnLoadLump = [&](int lumpId, BaseLump *pLump) {
        const BSPLump &lumpInfo = bspHeader.lump[lumpId];

        if (lumpInfo.nOffset + lumpInfo.nLength > (int32_t)data.size()) {
            throw LevelFormatException(fmt::format("Lump {}: invalid size/offset.", lumpId));
        }

        pLump->loadLump(data.subspan(lumpInfo.nOffset, lumpInfo.nLength));
        m_Lumps[lumpId].reset(pLump);
    };

    fnLoadLump(LUMP_PLANES, new PlanesLump());
    fnLoadLump(LUMP_VERTICES, new VerticesLump());
    fnLoadLump(LUMP_NODES, new NodesLump());
    fnLoadLump(LUMP_FACES, new FacesLump());
    fnLoadLump(LUMP_LEAVES, new LeavesLump());
    fnLoadLump(LUMP_EDGES, new EdgesLump());
    fnLoadLump(LUMP_SURFEDGES, new SurfEdgesLump());
}
