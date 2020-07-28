#include <appfw/services.h>
#include <renderer/base_renderer.h>

appfw::console::ConVar<int> r_cull("r_cull", 1,
                                   "Backface culling:\n"
                                   "0 - none\n"
                                   "1 - back\n"
                                   "2 - front",
                                   [](const int &, const int &newVal) {
                                       if (newVal < 0 || newVal > 2)
                                           return false;
                                       return true;
                                   });

appfw::console::ConVar<bool> r_drawworld("r_drawworld", true, "Draw world polygons");
appfw::console::ConVar<bool> r_lockpvs("r_lockpvs", false, "Lock current PVS to let devs see where it ends");
appfw::console::ConVar<bool> r_novis("r_novis", false, "Ignore visibility data");
appfw::console::ConVar<int> r_fullbright("r_fullbright", 0, "Disable lighting");

static uint8_t s_NoVis[bsp::MAX_MAP_LEAFS / 8];

//----------------------------------------------------------------
// LevelNode
//----------------------------------------------------------------
LevelNode::LevelNode(const bsp::Level &level, const bsp::BSPNode &bspNode) {
    pPlane = &level.getPlanes().at(bspNode.iPlane);
    children[0] = children[1] = nullptr;
    iFirstSurface = bspNode.firstFace;
    iNumSurfaces = bspNode.nFaces;
}

//----------------------------------------------------------------
// LevelLeaf
//----------------------------------------------------------------
LevelLeaf::LevelLeaf(const bsp::Level &level, const bsp::BSPLeaf &bspLeaf) {
    AFW_ASSERT(bspLeaf.nContents < 0);
    nContents = bspLeaf.nContents;

    if (bspLeaf.nVisOffset != -1 && !level.getVisData().empty()) {
        pCompressedVis = &level.getVisData().at(bspLeaf.nVisOffset);
    }
}

static inline float planeDiff(glm::vec3 point, const bsp::BSPPlane &plane) {
    float res = 0;

    switch (plane.nType) {
    case bsp::PlaneType::PlaneX:
        res = point[0];
        break;
    case bsp::PlaneType::PlaneY:
        res = point[1];
        break;
    case bsp::PlaneType::PlaneZ:
        res = point[2];
        break;
    default:
        res = glm::dot(point, plane.vNormal);
        break;
    }

    return res - plane.fDist;
}

BaseRenderer::BaseRenderer() {
    memset(m_DecompressedVis.data(), 0, m_DecompressedVis.size());
    memset(s_NoVis, 0xFF, sizeof(s_NoVis));
}

//----------------------------------------------------------------
// Loading
//----------------------------------------------------------------
void BaseRenderer::setLevel(const bsp::Level *level) {
    if (m_pLevel) {
        destroySurfaces();
        m_Leaves.clear();
        m_Nodes.clear();
        m_BaseSurfaces.clear();
        m_pViewLeaf = nullptr;
        m_pOldViewLeaf = nullptr;
        m_iFrame = 0;
        m_iVisFrame = 0;
    }

    m_pLevel = level;

    if (level) {
        try {
            createBaseSurfaces();
            createLeaves();
            createNodes();
        } catch (...) {
            m_pLevel = nullptr;
            throw;
        }
    }
}

void BaseRenderer::createBaseSurfaces() { 
    AFW_ASSERT(m_BaseSurfaces.empty());
    auto &lvlFaces = getLevel().getFaces();
    auto &lvlSurfEdges = getLevel().getSurfEdges();
    m_BaseSurfaces.resize(lvlFaces.size());

    for (size_t i = 0; i < lvlFaces.size(); i++) {
        const bsp::BSPFace &face = lvlFaces[i];
        LevelSurface &surface = m_BaseSurfaces[i];

        if (face.iFirstEdge + face.nEdges > (uint32_t)lvlSurfEdges.size()) {
            logWarn("createBaseSurfaces(): Bad surface {}", i);
            continue;
        }

        surface.iFirstEdge = face.iFirstEdge;
        surface.iNumEdges = face.nEdges;
        surface.pPlane = &getLevel().getPlanes().at(face.iPlane);

        if (face.nPlaneSide) {
            surface.iFlags |= SURF_PLANEBACK;
        }

        // Create vertices
        auto &lvlVertices = getLevel().getVertices();
        for (int j = 0; j < surface.iNumEdges; j++) {
            if (j == MAX_SIDE_VERTS) {
                logWarn("createBaseSurfaces(): polygon {} is too large (exceeded {} vertices)", j, MAX_SIDE_VERTS);
                break;
            }

            glm::vec3 vertex;
            bsp::BSPSurfEdge iEdgeIdx = lvlSurfEdges[(size_t)surface.iFirstEdge + j];

            if (iEdgeIdx > 0) {
                const bsp::BSPEdge &edge = getLevel().getEdges().at(iEdgeIdx);
                vertex = lvlVertices.at(edge.iVertex[0]);
            } else {
                const bsp::BSPEdge &edge = getLevel().getEdges().at(-iEdgeIdx);
                vertex = lvlVertices.at(edge.iVertex[1]);
            }

            surface.vertices.push_back(vertex);
        }

        surface.vertices.shrink_to_fit();

        // Find material
        const bsp::BSPTextureInfo &texInfo = getLevel().getTexInfo().at(face.iTextureInfo);
        const bsp::BSPMipTex &tex = getLevel().getTextures().at(texInfo.iMiptex);
        surface.pTexInfo = &texInfo;
        surface.nMatIndex = MaterialManager::get().findMaterial(tex.szName);
    }

    m_BaseSurfaces.shrink_to_fit();

    createSurfaces();
}

void BaseRenderer::createLeaves() {
    AFW_ASSERT(m_Leaves.empty());
    auto &lvlLeaves = getLevel().getLeaves();

    m_Leaves.reserve(lvlLeaves.size());

    for (size_t i = 0; i < lvlLeaves.size(); i++) {
        m_Leaves.emplace_back(getLevel(), lvlLeaves[i]);
    }
}

void BaseRenderer::createNodes() {
    AFW_ASSERT(m_Nodes.empty());
    auto &lvlNodes = getLevel().getNodes();

    m_Nodes.reserve(lvlNodes.size());

    for (size_t i = 0; i < lvlNodes.size(); i++) {
        m_Nodes.emplace_back(getLevel(), lvlNodes[i]);
    }

    // Set up children
    for (size_t i = 0; i < lvlNodes.size(); i++) {
        for (int j = 0; j < 2; j++) {
            int childNodeIdx = lvlNodes[i].iChildren[j];

            if (childNodeIdx >= 0) {
                m_Nodes[i].children[j] = &m_Nodes.at(childNodeIdx);
            } else {
                childNodeIdx = ~childNodeIdx;
                m_Nodes[i].children[j] = &m_Leaves.at(childNodeIdx);
            }
        }
    }

    updateNodeParents(&m_Nodes[0], nullptr);
}

void BaseRenderer::updateNodeParents(LevelNodeBase *node, LevelNodeBase *parent) {
    node->pParent = parent;

    if (node->nContents < 0) {
        return;
    }

    LevelNode *realNode = static_cast<LevelNode *>(node);
    updateNodeParents(realNode->children[0], node);
    updateNodeParents(realNode->children[1], node);
}

//----------------------------------------------------------------
// Rendering
//----------------------------------------------------------------
BaseRenderer::DrawStats BaseRenderer::draw(const DrawOptions &options) noexcept {
    if (!m_pLevel) {
        return DrawStats();
    }

    m_DrawStats = DrawStats();
    m_pOptions = &options;
    
    // Prepare data
    m_iFrame++;
    m_pOldViewLeaf = m_pViewLeaf;
    m_pViewLeaf = pointInLeaf(m_pOptions->viewOrigin);

    if (r_drawworld.getValue()) {
        markLeaves();
        drawWorld();
    }

    // Draw everything
    if (r_drawworld.getValue()) {
        drawWorldSurfaces(m_WorldSurfacesToDraw);
    }

    m_pOptions = nullptr;
    return m_DrawStats;
}

bool BaseRenderer::cullBox(glm::vec3 /* mins */, glm::vec3 /* maxs */) noexcept {
    if (r_cull.getValue() == 0) {
        return false;
    }

    // TODO: Frustum culling
    return false;
}

LevelLeaf *BaseRenderer::pointInLeaf(glm::vec3 p) noexcept {
    LevelNodeBase *pBaseNode = &m_Nodes[0];

    for (;;) {
        if (pBaseNode->nContents < 0) {
            return static_cast<LevelLeaf *>(pBaseNode);
        }

        LevelNode *pNode = static_cast<LevelNode *>(pBaseNode);
        const bsp::BSPPlane &plane = *pNode->pPlane;
        float d = glm::dot(p, plane.vNormal) - plane.fDist;

        if (d > 0) {
            pBaseNode = pNode->children[0];
        } else {
            pBaseNode = pNode->children[1];
        }
    }
}

uint8_t *BaseRenderer::leafPVS(LevelLeaf *pLeaf) noexcept {
    if (pLeaf == &m_Leaves[0]) {
        return s_NoVis;
    }

    // Decompress VIS
    int row = ((int)m_Leaves.size() + 7) >> 3;
    const uint8_t *in = pLeaf->pCompressedVis;
    uint8_t *out = m_DecompressedVis.data();

    if (!in) {
        // no vis info, so make all visible
        while (row) {
            *out++ = 0xff;
            row--;
        }
        return m_DecompressedVis.data();
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
    } while (out - m_DecompressedVis.data() < row);

    return m_DecompressedVis.data();
}

void BaseRenderer::markLeaves() noexcept {
    if (m_pOldViewLeaf == m_pViewLeaf && !r_novis.getValue()) {
        return;
    }

    if (r_lockpvs.getValue()) {
        return;
    }

    m_iVisFrame++;
    m_pOldViewLeaf = m_pViewLeaf;

    uint8_t *vis = nullptr;
    uint8_t solid[4096];

    if (r_novis.getValue()) {
        AFW_ASSERT(((m_Leaves.size() + 7) >> 3) <= sizeof(solid));
        vis = solid;
        memset(solid, 0xFF, (m_Leaves.size() + 7) >> 3);
    } else {
        vis = leafPVS(m_pViewLeaf);
    }

    for (size_t i = 0; i < m_Leaves.size() - 1; i++) {
        if (vis[i >> 3] & (1 << (i & 7))) {
            LevelNodeBase *node = &m_Leaves[i + 1];

            do {
                if (node->iVisFrame == m_iVisFrame)
                    break;
                node->iVisFrame = m_iVisFrame;
                node = node->pParent;
            } while (node);
        }
    }
}

bool BaseRenderer::cullSurface(const LevelSurface &pSurface) noexcept {
    if (r_cull.getValue() == 0) {
        return false;
    }

    float dist = planeDiff(m_pOptions->viewOrigin, *pSurface.pPlane);

    if (r_cull.getValue() == 1) {
        // Back face culling
        if (pSurface.iFlags & SURF_PLANEBACK) {
            if (dist >= -BACKFACE_EPSILON)
                return true; // wrong side
        } else {
            if (dist <= BACKFACE_EPSILON)
                return true; // wrong side
        }
    } else if (r_cull.getValue() == 2) {
        // Front face culling
        if (pSurface.iFlags & SURF_PLANEBACK) {
            if (dist <= BACKFACE_EPSILON)
                return true; // wrong side
        } else {
            if (dist >= -BACKFACE_EPSILON)
                return true; // wrong side
        }
    }

    // TODO: Frustum culling
    return false;
}

void BaseRenderer::drawWorld() noexcept {
    m_WorldSurfacesToDraw.clear();
    recursiveWorldNodes(&m_Nodes[0]);
}

void BaseRenderer::recursiveWorldNodes(LevelNodeBase *pNodeBase) noexcept {
    if (pNodeBase->iVisFrame != m_iVisFrame) {
        return;
    }

    if (pNodeBase->nContents < 0) {
        // Leaf
        return;
    }

    LevelNode *pNode = static_cast<LevelNode *>(pNodeBase);

    float dot = planeDiff(m_pOptions->viewOrigin, *pNode->pPlane);
    int side = (dot >= 0.0f) ? 0 : 1;

    // Front side
    recursiveWorldNodes(pNode->children[side]);

    for (int i = 0; i < pNode->iNumSurfaces; i++) {
        LevelSurface &surface = m_BaseSurfaces[(size_t)pNode->iFirstSurface + i];

        if (cullSurface(surface)) {
            continue;
        }

        m_WorldSurfacesToDraw.push_back((size_t)pNode->iFirstSurface + i);
    }

    // Back side
    recursiveWorldNodes(pNode->children[!side]);
}
