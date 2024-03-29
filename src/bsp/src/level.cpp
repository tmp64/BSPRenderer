#include <fstream>
#include <vector>
#include <appfw/appfw.h>
#include <bsp/level.h>
#include <fmt/format.h>

using namespace std::literals::string_literals;

namespace {

class NoVis {
public:
    uint8_t data[bsp::MAX_MAP_LEAFS / 8];

    NoVis() { memset(data, 0xFF, sizeof(data)); }
};

NoVis s_NoVis;

}

bsp::Level::Level(appfw::span<uint8_t> data) { loadFromBytes(data); }

bsp::Level::Level(const std::string &filename) { loadFromFile(filename); }

void bsp::Level::loadFromFile(const fs::path &filename) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (file.fail()) {
        throw std::runtime_error(std::string("Failed to open file: ") + strerror(errno));
    }

    std::vector<uint8_t> data;
    appfw::readFileContents(file, data);
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
        using Container = typename std::remove_reference<decltype(storage)>::type;
        using DataType = typename Container::value_type;
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
    fnLoadLump(LUMP_TEXTURES, m_RawTextureLump);
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

    if (entLump[entLump.size() - 1] == '\0') {
        m_Entities = std::string(entLump.data(), entLump.size() - 1);
    } else {
        m_Entities = std::string(entLump.data(), entLump.size());
    }
}

std::vector<glm::vec3> bsp::Level::getFaceVertices(const bsp::BSPFace &face) const {
    std::vector<glm::vec3> verts;
    auto &lvlVertices = getVertices();
    auto &lvlSurfEdges = getSurfEdges();

    for (int j = 0; j < face.nEdges; j++) {
        glm::vec3 vertex;
        bsp::BSPSurfEdge iEdgeIdx = lvlSurfEdges[(size_t)face.iFirstEdge + j];

        if (iEdgeIdx > 0) {
            const bsp::BSPEdge &edge = getEdges()[iEdgeIdx];
            vertex = lvlVertices[edge.iVertex[0]];
        } else {
            const bsp::BSPEdge &edge = getEdges()[-iEdgeIdx];
            vertex = lvlVertices[edge.iVertex[1]];
        }

        verts.push_back(vertex);
    }

    return verts;
}

int bsp::Level::traceLine(glm::vec3 from, glm::vec3 to) const {
    return recursiveTraceLine(0, from, to);
}

int bsp::Level::pointInLeaf(glm::vec3 p) const noexcept {
    int node = 0;

    for (;;) {
        if (node < 0) {
            return node;
        }

        const bsp::BSPNode *pNode = &m_Nodes[node];
        const bsp::BSPPlane &plane = m_Planes[pNode->iPlane];
        float d = glm::dot(p, plane.vNormal) - plane.fDist;

        if (d > 0) {
            node = pNode->iChildren[0];
        } else {
            node = pNode->iChildren[1];
        }
    }
}

const uint8_t *bsp::Level::leafPVS(int leaf, appfw::span<uint8_t> buf) const noexcept {
    AFW_ASSERT(leaf < 0);
    AFW_ASSERT(buf.size() >= bsp::MAX_MAP_LEAFS / 8);
    int leafIdx = ~leaf;

    if (leafIdx == 0) {
        return s_NoVis.data;
    }

    const bsp::BSPLeaf *pLeaf = (leaf != 0) ? &m_Leaves[leafIdx] : nullptr;

    // Decompress VIS
    int row = ((int)m_Leaves.size() + 7) >> 3;
    const uint8_t *in = nullptr;
    uint8_t *out = buf.data();

    glm::vec3 v;
    v[0] = 1;

    if (!pLeaf || pLeaf->nVisOffset == -1) {
        in = nullptr;
    } else {
        in = m_VisData.data() + pLeaf->nVisOffset;
    }

    if (!in) {
        // No vis info, so make all visible
        while (row) {
            *out++ = 0xff;
            row--;
        }
        return buf.data();
    }

    do {
        if (*in) {
            *out++ = *in++;
            continue;
        }

        int c = in[1];
        in += 2;

        while (c) {
            *out++ = 0;
            c--;
        }
    } while (out - buf.data() < row);

    return buf.data();
}

int bsp::Level::recursiveTraceLine(int nodeidx, const glm::vec3 &start, const glm::vec3 &stop) const {
    // Based on VDC article:
    // https://developer.valvesoftware.com/wiki/BSP#How_are_BSP_trees_used_for_collision_detection.3F
    // and qrad code from HLSDK.
    constexpr float ON_EPSILON = 0.025f;

    if (nodeidx < 0) {
        const BSPLeaf &leaf = getLeaves()[~nodeidx];

        if (leaf.nContents == CONTENTS_SOLID) {
            return CONTENTS_SOLID;
        } else if (leaf.nContents == CONTENTS_SKY) {
            return CONTENTS_SKY;
        } else {
            return CONTENTS_EMPTY;
        }
    }

    const BSPNode &node = getNodes()[nodeidx];
    const BSPPlane &plane = getPlanes()[node.iPlane];

    // Code from qrad/trace.c TestLine_r
    float front, back;
    switch (plane.nType) {
    case bsp::PlaneType::PlaneX:
        front = start[0] - plane.fDist;
        back = stop[0] - plane.fDist;
        break;
    case bsp::PlaneType::PlaneY:
        front = start[1] - plane.fDist;
        back = stop[1] - plane.fDist;
        break;
    case bsp::PlaneType::PlaneZ:
        front = start[2] - plane.fDist;
        back = stop[2] - plane.fDist;
        break;
    default:
        front = glm::dot(start, plane.vNormal) - plane.fDist;
        back = glm::dot(stop, plane.vNormal) - plane.fDist;
        break;
    }

    if (front >= -ON_EPSILON && back >= -ON_EPSILON)
        return recursiveTraceLine(node.iChildren[0], start, stop);

    if (front < ON_EPSILON && back < ON_EPSILON)
        return recursiveTraceLine(node.iChildren[1], start, stop);

    int side = front < 0;

    float frac = front / (front - back);
    glm::vec3 mid = start + (stop - start) * frac;

    int r = recursiveTraceLine(node.iChildren[side], start, mid);

    if (r != CONTENTS_EMPTY) {
        return r;
    }
    
    return recursiveTraceLine(node.iChildren[!side], mid, stop);
}
