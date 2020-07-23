#include <appfw/services.h>
#include <renderer/base_renderer.h>

LevelNode::LevelNode(const bsp::Level &level, const bsp::BSPNode &bspNode) {
    pPlane = &level.getPlanes().at(bspNode.iPlane);
    children[0] = children[1] = nullptr;
    iFirstSurface = bspNode.firstFace;
    iNumSurfaces = bspNode.nFaces;
}

LevelLeaf::LevelLeaf(const bsp::Level &/* level */, const bsp::BSPLeaf &bspLeaf) {
    AFW_ASSERT(bspLeaf.nContents < 0);
    nContents = bspLeaf.nContents;
}

static inline float planeDiff(glm::vec3 point, const bsp::BSPPlane &plane) {
    float res = 0;

    switch (plane.nType) {
    case bsp::PlaneType::PlaneX:
        res = point[0];
    case bsp::PlaneType::PlaneY:
        res = point[1];
    case bsp::PlaneType::PlaneZ:
        res = point[2];
    default:
        res = glm::dot(point, plane.vNormal);
    }

    return res - plane.fDist;
}

void BaseRenderer::setLevel(const bsp::Level *level) {
    if (m_pLevel) {
        destroySurfaces();
        m_Leaves.clear();
        m_Nodes.clear();
        m_BaseSurfaces.clear();
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

BaseRenderer::DrawStats BaseRenderer::draw(const DrawOptions &options) { 
	if (!m_pLevel) {
        return DrawStats();
	}

    m_DrawStats = DrawStats();
    m_pOptions = &options;
    m_WorldSurfacesToDraw.clear();

    recursiveWorldNodes(m_Nodes[0].get());
    drawWorldSurfaces(m_WorldSurfacesToDraw);

    m_pOptions = nullptr;
    return m_DrawStats;
}

LevelLeaf *BaseRenderer::createLeaf(const bsp::BSPLeaf &bspLeaf) {
    return new LevelLeaf(getLevel(), bspLeaf);
}

LevelNode *BaseRenderer::createNode(const bsp::BSPNode &bspNode) {
    return new LevelNode(getLevel(), bspNode);
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
    }

    m_BaseSurfaces.shrink_to_fit();

    createSurfaces();
}

void BaseRenderer::createLeaves() {
    AFW_ASSERT(m_Leaves.empty());
    auto &lvlLeaves = getLevel().getLeaves();

    m_Leaves.resize(lvlLeaves.size());

    for (size_t i = 0; i < lvlLeaves.size(); i++) {
        m_Leaves[i].reset(createLeaf(lvlLeaves[i]));
    }

    m_Leaves.shrink_to_fit();
}

void BaseRenderer::createNodes() {
    AFW_ASSERT(m_Nodes.empty());
    auto &lvlNodes = getLevel().getNodes();

    m_Nodes.resize(lvlNodes.size());

    for (size_t i = 0; i < lvlNodes.size(); i++) {
        m_Nodes[i].reset(createNode(lvlNodes[i]));
    }

    // Set up children
    for (size_t i = 0; i < lvlNodes.size(); i++) {
        for (int j = 0; j < 2; j++) {
            int childNodeIdx = lvlNodes[i].iChildren[j];

            if (childNodeIdx >= 0) {
                m_Nodes[i]->children[j] = m_Nodes[childNodeIdx].get();
            } else {
                childNodeIdx = ~childNodeIdx;
                m_Nodes[i]->children[j] = m_Leaves[childNodeIdx].get();
            }
        }
    }

    m_Nodes.shrink_to_fit();

    updateNodeParents(m_Nodes[0].get(), nullptr);
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

void BaseRenderer::recursiveWorldNodes(LevelNodeBase *pNodeBase) {

    if (pNodeBase->nContents < 0) {
        // Leaf
        return;
    }

    LevelNode *pNode = static_cast<LevelNode *>(pNodeBase);

    float dot = planeDiff(m_pOptions->viewOrigin, *pNode->pPlane);
    int side = (dot >= 0.0f) ? 0 : 1;

    recursiveWorldNodes(pNode->children[side]);

    for (int i = 0; i < pNode->iNumSurfaces; i++) {
        m_WorldSurfacesToDraw.push_back((size_t)pNode->iFirstSurface + i);
    }

    recursiveWorldNodes(pNode->children[!side]);
}
