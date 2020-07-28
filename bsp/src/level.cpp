#include <fstream>
#include <vector>
#include <bsp/level.h>
#include <fmt/format.h>

using namespace std::literals::string_literals;

bsp::Level::Level(appfw::span<uint8_t> data) { loadFromBytes(data); }

bsp::Level::Level(const std::string &filename) { loadFromFile(filename); }

void bsp::Level::loadFromFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (file.fail()) {
        throw std::runtime_error(std::string("Failed to open file: ") + strerror(errno));
    }

    std::vector<uint8_t> data;
    data.reserve(64 * 1024 * 1024); // Set capacity to 4 MB

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
    auto fnLoadLump = [&](int lumpId, auto &storage) {
        using Container = std::remove_reference<decltype(storage)>::type;
        using DataType = Container::value_type;
        const BSPLump &lumpInfo = bspHeader.lump[lumpId];

        if (lumpInfo.nOffset + lumpInfo.nLength > (int32_t)data.size()) {
            throw LevelFormatException(fmt::format("{}: invalid size/offset", bsp::LUMP_NAME[lumpId]));
        }

        appfw::span<uint8_t> lumpData = data.subspan(lumpInfo.nOffset, lumpInfo.nLength);

        if (lumpData.size() % sizeof(DataType) != 0) {
            throw LevelFormatException(fmt::format("{}: incorrect size", bsp::LUMP_NAME[lumpId]));
        }

        size_t count = lumpData.size() / sizeof(DataType);
        storage.resize(count);
        memcpy(storage.data(), lumpData.data(), lumpData.size());
    };

    auto fnLoadTextures = [&]() {
        const BSPLump &lumpInfo = bspHeader.lump[LUMP_TEXTURES];

        if (lumpInfo.nOffset + lumpInfo.nLength > (int32_t)data.size()) {
            throw LevelFormatException(fmt::format("LUMP_TEXTURES: invalid size/offset."));
        }

        appfw::span<uint8_t> lumpData = data.subspan(lumpInfo.nOffset, lumpInfo.nLength);

        // Copy header
        BSPTextureHeader header;

        if (sizeof(header) > lumpData.size()) {
            throw LevelFormatException(fmt::format("LUMP_TEXTURES: size too small for header"));
        }

        memcpy(&header, lumpData.data(), sizeof(header));

        // Copy offsets
        std::vector<BSPMipTexOffset> offsets;
        offsets.resize(header.nMipTextures);

        if (sizeof(header) + sizeof(BSPMipTexOffset) * header.nMipTextures > lumpData.size()) {
            throw LevelFormatException(fmt::format("LUMP_TEXTURES: size too small for offsets"));
        }

        memcpy(offsets.data(), lumpData.data() + sizeof(header), sizeof(BSPMipTexOffset) * header.nMipTextures);

        // Copy textures
        m_Textures.resize(header.nMipTextures);

        for (size_t i = 0; i < m_Textures.size(); i++) {
            size_t offset = offsets[i];
            
            if (offset + sizeof(BSPMipTex) > lumpData.size()) {
                throw LevelFormatException(fmt::format("LUMP_TEXTURES: size too small for BSPMipTex"));
            }

            memcpy(m_Textures.data() + i, lumpData.data() + offset, sizeof(BSPMipTex));
            m_Textures[i].szName[MAX_TEXTURE_NAME - 1] = '\0';
        }
    };

    //fnLoadLump(LUMP_ENTITIES, ???);
    fnLoadLump(LUMP_PLANES, m_Planes);
    fnLoadTextures();
    fnLoadLump(LUMP_VERTICES, m_Vertices);
    fnLoadLump(LUMP_VISIBILITY, m_VisData);
    fnLoadLump(LUMP_NODES, m_Nodes);
    fnLoadLump(LUMP_TEXINFO, m_TexInfo);
    fnLoadLump(LUMP_FACES, m_Faces);
    fnLoadLump(LUMP_LIGHTING, m_Ligthmaps);
    fnLoadLump(LUMP_LEAVES, m_Leaves);
    fnLoadLump(LUMP_MARKSURFACES, m_MarkSurfaces);
    fnLoadLump(LUMP_EDGES, m_Edges);
    fnLoadLump(LUMP_SURFEDGES, m_SurfEdges);
    fnLoadLump(LUMP_MODELS, m_Models);
}
