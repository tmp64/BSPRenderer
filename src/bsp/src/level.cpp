#include <fstream>
#include <vector>
#include <appfw/services.h>
#include <bsp/level.h>
#include <fmt/format.h>

using namespace std::literals::string_literals;

bsp::Level::EntityList::EntityList(const std::vector<char> &entityLump) {
    size_t i = 0;

    auto fnSkipWhitespace = [&]() {
        auto &c = entityLump;
        while (c[i] == ' ' || c[i] == '\r' || c[i] == '\n' || c[i] == '\t') {
            i++;
        }
    };

    auto fnCheckEOF = [&]() {
        if (i >= entityLump.size()) {
            throw LevelFormatException("Entity lump: unexpected end of file at position " + std::to_string(i));
        }
    };

    while (i < entityLump.size()) {
        fnSkipWhitespace();

        if (entityLump[i] == '\0') {
            // Finished parsing
            break;
        }

        if (entityLump[i] != '{') {
            throw LevelFormatException("Entity lump: expected '{' at position " + std::to_string(i));
        }
        i++;

        EntityListItem item;

        while (i < entityLump.size()) {
            fnSkipWhitespace();

            // "key"
            if (entityLump[i] != '}' && entityLump[i] != '"') {
                throw LevelFormatException("Entity lump: expected '\"' at position " + std::to_string(i));
            }

            if (entityLump[i] == '}') {
                break;
            }

            i++;
            size_t keyBegin = i;

            while (entityLump[i] != '"' && i < entityLump.size()) {
                i++;
            }

            fnCheckEOF();
            size_t keyEnd = i;
            i++;
            
            // "value"
            fnSkipWhitespace();
            if (entityLump[i] != '"') {
                throw LevelFormatException("Entity lump: expected '\"' at position " + std::to_string(i));
            }

            i++;
            size_t valueBegin = i;

            while (entityLump[i] != '"' && i < entityLump.size()) {
                i++;
            }

            fnCheckEOF();
            size_t valueEnd = i;
            i++;

            // Add into the item
            std::string_view key(&entityLump[keyBegin], keyEnd - keyBegin);
            std::string_view value(&entityLump[valueBegin], valueEnd - valueBegin);
            item.m_Keys.insert({std::string(key), std::string(value)});
        }

        fnCheckEOF();

        if (entityLump[i] != '}') {
            throw LevelFormatException("Entity lump: expected '}' at position " + std::to_string(i));
        }
        i++;

        // Add item into the list
        m_Items.push_back(std::move(item));
    }

    // Find worldspawn
    for (EntityListItem &item : m_Items) {
        if (item.getValue<std::string>("classname") == "worldspawn") {
            if (m_pWorldspawn) {
                throw LevelFormatException("Multiple worldspawns found");
            } else {
                m_pWorldspawn = &item;
            }
        }
    }
}

const bsp::Level::EntityListItem *bsp::Level::EntityList::findEntityByName(const std::string &targetname) const {
    for (const EntityListItem &i : m_Items) {
        if (i.getValue<std::string>("targetname") == targetname) {
            return &i;
        }
    }

    return nullptr;
}

bsp::Level::Level(appfw::span<uint8_t> data) { loadFromBytes(data); }

bsp::Level::Level(const std::string &filename) { loadFromFile(filename); }

void bsp::Level::loadFromFile(const fs::path &filename) {
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

    fnLoadLump(LUMP_PLANES, m_Planes);
    fnLoadTextures();
    fnLoadLump(LUMP_VERTICES, m_Vertices);
    fnLoadLump(LUMP_VISIBILITY, m_VisData);
    fnLoadLump(LUMP_NODES, m_Nodes);
    fnLoadLump(LUMP_TEXINFO, m_TexInfo);
    fnLoadLump(LUMP_FACES, m_Faces);
    fnLoadLump(LUMP_LIGHTING, m_Lightmaps);
    fnLoadLump(LUMP_LEAVES, m_Leaves);
    fnLoadLump(LUMP_MARKSURFACES, m_MarkSurfaces);
    fnLoadLump(LUMP_EDGES, m_Edges);
    fnLoadLump(LUMP_SURFEDGES, m_SurfEdges);
    fnLoadLump(LUMP_MODELS, m_Models);

    if (m_Lightmaps.size() % 3 != 0) {
        throw LevelFormatException(fmt::format("LUMP_LIGHTING: invalid size"));
    }

    // Load entities
    std::vector<char> entLump;
    fnLoadLump(LUMP_ENTITIES, entLump);
    entLump.push_back('\0');
    m_Entities = EntityList(entLump);
}
