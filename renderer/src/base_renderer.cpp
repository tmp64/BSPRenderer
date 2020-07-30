#include <appfw/services.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <renderer/base_renderer.h>
#include <renderer/utils.h>

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
appfw::console::ConVar<bool> r_no_frustum_culling("r_no_frustum_culling", true, "Disable frustum culling");

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
    memset(getFrameVars().decompressedVis.data(), 0, getFrameVars().decompressedVis.size());
    memset(s_NoVis, 0xFF, sizeof(s_NoVis));
}

//----------------------------------------------------------------
// Loading
//----------------------------------------------------------------
void BaseRenderer::setLevel(const bsp::Level *level) {
    if (m_pLevel) {
        destroySurfaces();
        getFrameVars() = FrameVars();
        getLevelVars() = LevelVars();
    }

    m_pLevel = level;

    if (level) {
        try {
            createBaseSurfaces();
            createLeaves();
            createNodes();
        } catch (...) {
            // Clean up everything created
            setLevel(nullptr);
            throw;
        }
    }
}

void BaseRenderer::createBaseSurfaces() { 
    AFW_ASSERT(getLevelVars().baseSurfaces.empty());
    auto &lvlFaces = getLevel().getFaces();
    auto &lvlSurfEdges = getLevel().getSurfEdges();
    getLevelVars().baseSurfaces.resize(lvlFaces.size());

    for (size_t i = 0; i < lvlFaces.size(); i++) {
        const bsp::BSPFace &face = lvlFaces[i];
        LevelSurface &surface = getLevelVars().baseSurfaces[i];

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

        // Create vertices and calculate bounds
        surface.mins[0] = surface.mins[1] = surface.mins[2] = 999999.0f;
        surface.maxs[0] = surface.maxs[1] = surface.maxs[2] = -999999.0f;
        auto &lvlVertices = getLevel().getVertices();

        for (int j = 0; j < surface.iNumEdges; j++) {
            if (j == MAX_SIDE_VERTS) {
                logWarn("createBaseSurfaces(): polygon {} is too large (exceeded {} vertices)", j, MAX_SIDE_VERTS);
                break;
            }

            glm::vec3 vertex;
            bsp::BSPSurfEdge iEdgeIdx = lvlSurfEdges.at((size_t)surface.iFirstEdge + j);

            if (iEdgeIdx > 0) {
                const bsp::BSPEdge &edge = getLevel().getEdges().at(iEdgeIdx);
                vertex = lvlVertices.at(edge.iVertex[0]);
            } else {
                const bsp::BSPEdge &edge = getLevel().getEdges().at(-iEdgeIdx);
                vertex = lvlVertices.at(edge.iVertex[1]);
            }

            surface.vertices.push_back(vertex);

            // Add vertex to bounds
            for (int k = 0; k < 3; k++) {
                float val = vertex[k];
                if (val < surface.mins[k])
                    surface.mins[k] = val;
                if (val > surface.maxs[k])
                    surface.maxs[k] = val;
            }
        }

        surface.vertices.shrink_to_fit();

        // Find material
        const bsp::BSPTextureInfo &texInfo = getLevel().getTexInfo().at(face.iTextureInfo);
        const bsp::BSPMipTex &tex = getLevel().getTextures().at(texInfo.iMiptex);
        surface.pTexInfo = &texInfo;
        surface.nMatIndex = MaterialManager::get().findMaterial(tex.szName);
    }

    getLevelVars().baseSurfaces.shrink_to_fit();

    createSurfaces();
}

void BaseRenderer::createLeaves() {
    AFW_ASSERT(getLevelVars().leaves.empty());
    auto &lvlLeaves = getLevel().getLeaves();

    getLevelVars().leaves.reserve(lvlLeaves.size());

    for (size_t i = 0; i < lvlLeaves.size(); i++) {
        getLevelVars().leaves.emplace_back(getLevel(), lvlLeaves[i]);
    }
}

void BaseRenderer::createNodes() {
    AFW_ASSERT(getLevelVars().nodes.empty());
    auto &lvlNodes = getLevel().getNodes();

    getLevelVars().nodes.reserve(lvlNodes.size());

    for (size_t i = 0; i < lvlNodes.size(); i++) {
        getLevelVars().nodes.emplace_back(getLevel(), lvlNodes[i]);
    }

    // Set up children
    for (size_t i = 0; i < lvlNodes.size(); i++) {
        for (int j = 0; j < 2; j++) {
            int childNodeIdx = lvlNodes[i].iChildren[j];

            if (childNodeIdx >= 0) {
                getLevelVars().nodes[i].children[j] = &getLevelVars().nodes.at(childNodeIdx);
            } else {
                childNodeIdx = ~childNodeIdx;
                getLevelVars().nodes[i].children[j] = &getLevelVars().leaves.at(childNodeIdx);
            }
        }
    }

    updateNodeParents(&getLevelVars().nodes[0], nullptr);
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
    setupFrame();

    if (r_drawworld.getValue()) {
        markLeaves();
        drawWorld();
    }

    // Draw everything
    if (r_drawworld.getValue()) {
        drawWorldSurfaces(getFrameVars().worldSurfacesToDraw);
    }

    m_pOptions = nullptr;

    return m_DrawStats;
}

bool BaseRenderer::cullBox(glm::vec3 mins, glm::vec3 maxs) noexcept {
    if (r_cull.getValue() == 0 || r_no_frustum_culling.getValue()) {
        return false;
    }

    unsigned i;
    const Plane *p;

    for (i = (unsigned)getFrameVars().frustum.size(), p = &getFrameVars().frustum[0]; i > 0; i--, p++) {
        switch (p->signbits) {
        case 0:
            if (p->vNormal[0] * maxs[0] + p->vNormal[1] * maxs[1] + p->vNormal[2] * maxs[2] < p->fDist)
                return true;
            break;
        case 1:
            if (p->vNormal[0] * mins[0] + p->vNormal[1] * maxs[1] + p->vNormal[2] * maxs[2] < p->fDist)
                return true;
            break;
        case 2:
            if (p->vNormal[0] * maxs[0] + p->vNormal[1] * mins[1] + p->vNormal[2] * maxs[2] < p->fDist)
                return true;
            break;
        case 3:
            if (p->vNormal[0] * mins[0] + p->vNormal[1] * mins[1] + p->vNormal[2] * maxs[2] < p->fDist)
                return true;
            break;
        case 4:
            if (p->vNormal[0] * maxs[0] + p->vNormal[1] * maxs[1] + p->vNormal[2] * mins[2] < p->fDist)
                return true;
            break;
        case 5:
            if (p->vNormal[0] * mins[0] + p->vNormal[1] * maxs[1] + p->vNormal[2] * mins[2] < p->fDist)
                return true;
            break;
        case 6:
            if (p->vNormal[0] * maxs[0] + p->vNormal[1] * mins[1] + p->vNormal[2] * mins[2] < p->fDist)
                return true;
            break;
        case 7:
            if (p->vNormal[0] * mins[0] + p->vNormal[1] * mins[1] + p->vNormal[2] * mins[2] < p->fDist)
                return true;
            break;
        default:
            return false;
        }
    }
    return false;
}

LevelLeaf *BaseRenderer::pointInLeaf(glm::vec3 p) noexcept {
    LevelNodeBase *pBaseNode = &getLevelVars().nodes[0];

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
    if (pLeaf == &getLevelVars().leaves[0]) {
        return s_NoVis;
    }

    // Decompress VIS
    int row = ((int)getLevelVars().leaves.size() + 7) >> 3;
    const uint8_t *in = pLeaf->pCompressedVis;
    uint8_t *out = getFrameVars().decompressedVis.data();

    if (!in) {
        // no vis info, so make all visible
        while (row) {
            *out++ = 0xff;
            row--;
        }
        return getFrameVars().decompressedVis.data();
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
    } while (out - getFrameVars().decompressedVis.data() < row);

    return getFrameVars().decompressedVis.data();
}

void BaseRenderer::setupFrame() noexcept {
    getFrameVars().viewOrigin = m_pOptions->viewOrigin;
    getFrameVars().viewAngles = m_pOptions->viewAngles;

    getLevelVars().iFrame++;
    getLevelVars().pOldViewLeaf = getFrameVars().pViewLeaf;
    getFrameVars().pViewLeaf = pointInLeaf(getFrameVars().viewOrigin);

    // Set up view matrix
    {
        getFrameVars().viewMat = glm::identity<glm::mat4>();
        getFrameVars().viewMat = glm::rotate(getFrameVars().viewMat, glm::radians(-90.f), {1.0f, 0.0f, 0.0f});
        getFrameVars().viewMat = glm::rotate(getFrameVars().viewMat, glm::radians(90.f), {0.0f, 0.0f, 1.0f});
        getFrameVars().viewMat = glm::rotate(getFrameVars().viewMat, glm::radians(-getFrameVars().viewAngles.z), {1.0f, 0.0f, 0.0f});
        getFrameVars().viewMat = glm::rotate(getFrameVars().viewMat, glm::radians(-getFrameVars().viewAngles.x), {0.0f, 1.0f, 0.0f});
        getFrameVars().viewMat = glm::rotate(getFrameVars().viewMat, glm::radians(-getFrameVars().viewAngles.y), {0.0f, 0.0f, 1.0f});
        getFrameVars().viewMat = glm::translate(getFrameVars().viewMat,
                                           {-getFrameVars().viewOrigin.x, -getFrameVars().viewOrigin.y, -getFrameVars().viewOrigin.z});
    }

    // Set up projection matrix
    {
        float fov_x = m_pOptions->fov;
        float aspect = m_pOptions->aspect;
        float fov_x_tan = 0;

        if (fov_x < 1.0) {
            fov_x_tan = 1.0;
        } else if (fov_x <= 179.0) {
            fov_x_tan = tan(fov_x * glm::pi<float>() / 360.f);
        } else {
            fov_x_tan = 1.0;
        }

        float fov_y = atanf(fov_x_tan / aspect) * 360.f / glm::pi<float>();
        float fov_y_tan = tan(fov_y * glm::pi<float>() / 360.f) * 4.f;

        float zNear = 4.0f;
        float zFar = std::max(256.0f, m_pOptions->far);

        float xMax = fov_y_tan * aspect;
        float xMin = -xMax;

        getFrameVars().projMat = glm::frustum(xMin, xMax, -fov_y_tan, fov_y_tan, zNear, zFar);

        m_FrameVars.fov_x = fov_x;
        m_FrameVars.fov_y = fov_y;
    }

    // Build the transformation matrix for the given view angles
    angleVectors(getFrameVars().viewAngles, &getFrameVars().vForward, &getFrameVars().vRight, &getFrameVars().vUp);

    // Setup viewplane dist
    getFrameVars().flViewPlaneDist = glm::dot(getFrameVars().viewOrigin, getFrameVars().vForward);

    // Setup frustum
    // rotate getFrameVars().vForward right by FOV_X/2 degrees
    getFrameVars().frustum[0].vNormal = rotatePointAroundVector(getFrameVars().vUp, getFrameVars().vForward, -(90 - m_FrameVars.fov_x / 2));
    // rotate getFrameVars().vForward left by FOV_X/2 degrees
    getFrameVars().frustum[1].vNormal = rotatePointAroundVector(getFrameVars().vUp, getFrameVars().vForward, 90 - m_FrameVars.fov_x / 2);
    // rotate getFrameVars().vForward up by FOV_X/2 degrees
    getFrameVars().frustum[2].vNormal = rotatePointAroundVector(getFrameVars().vRight, getFrameVars().vForward, 90 - m_FrameVars.fov_y / 2);
    // rotate getFrameVars().vForward down by FOV_X/2 degrees
    getFrameVars().frustum[3].vNormal = rotatePointAroundVector(getFrameVars().vRight, getFrameVars().vForward, -(90 - m_FrameVars.fov_y / 2));
    // negate forward vector
    getFrameVars().frustum[4].vNormal = -getFrameVars().vForward;

    for (size_t i = 0; i < 4; i++) {
        getFrameVars().frustum[i].nType = bsp::PlaneType::PlaneNonAxial;
        getFrameVars().frustum[i].fDist = glm::dot(getFrameVars().viewOrigin, getFrameVars().frustum[i].vNormal);
        getFrameVars().frustum[i].signbits = signbitsForPlane(getFrameVars().frustum[i].vNormal);
    }

    glm::vec3 farPoint = vectorMA(getFrameVars().viewOrigin, m_pOptions->far, getFrameVars().vForward);
    getFrameVars().frustum[5].nType = bsp::PlaneType::PlaneNonAxial;
    getFrameVars().frustum[5].fDist = glm::dot(farPoint, getFrameVars().frustum[5].vNormal);
    getFrameVars().frustum[5].signbits = signbitsForPlane(getFrameVars().frustum[5].vNormal);
}

void BaseRenderer::markLeaves() noexcept {
    if (getLevelVars().pOldViewLeaf == getFrameVars().pViewLeaf && !r_novis.getValue()) {
        return;
    }

    if (r_lockpvs.getValue()) {
        return;
    }

    getLevelVars().iVisFrame++;
    getLevelVars().pOldViewLeaf = getFrameVars().pViewLeaf;

    uint8_t *vis = nullptr;
    uint8_t solid[4096];

    if (r_novis.getValue()) {
        AFW_ASSERT(((getLevelVars().leaves.size() + 7) >> 3) <= sizeof(solid));
        vis = solid;
        memset(solid, 0xFF, (getLevelVars().leaves.size() + 7) >> 3);
    } else {
        vis = leafPVS(getFrameVars().pViewLeaf);
    }

    for (size_t i = 0; i < getLevelVars().leaves.size() - 1; i++) {
        if (vis[i >> 3] & (1 << (i & 7))) {
            LevelNodeBase *node = &getLevelVars().leaves[i + 1];

            do {
                if (node->iVisFrame == getLevelVars().iVisFrame)
                    break;
                node->iVisFrame = getLevelVars().iVisFrame;
                node = node->pParent;
            } while (node);
        }
    }
}

bool BaseRenderer::cullSurface(const LevelSurface &surface) noexcept {
    if (r_cull.getValue() == 0) {
        return false;
    }

    float dist = planeDiff(getFrameVars().viewOrigin, *surface.pPlane);

    if (r_cull.getValue() == 1) {
        // Back face culling
        if (surface.iFlags & SURF_PLANEBACK) {
            if (dist >= -BACKFACE_EPSILON)
                return true; // wrong side
        } else {
            if (dist <= BACKFACE_EPSILON)
                return true; // wrong side
        }
    } else if (r_cull.getValue() == 2) {
        // Front face culling
        if (surface.iFlags & SURF_PLANEBACK) {
            if (dist <= BACKFACE_EPSILON)
                return true; // wrong side
        } else {
            if (dist >= -BACKFACE_EPSILON)
                return true; // wrong side
        }
    }

    // TODO: Frustum culling
    return cullBox(surface.mins, surface.maxs);
}

void BaseRenderer::drawWorld() noexcept {
    getFrameVars().worldSurfacesToDraw.clear();
    recursiveWorldNodes(&getLevelVars().nodes[0]);
}

void BaseRenderer::recursiveWorldNodes(LevelNodeBase *pNodeBase) noexcept {
    if (pNodeBase->iVisFrame != getLevelVars().iVisFrame) {
        return;
    }

    if (pNodeBase->nContents < 0) {
        // Leaf
        return;
    }

    LevelNode *pNode = static_cast<LevelNode *>(pNodeBase);

    float dot = planeDiff(getFrameVars().viewOrigin, *pNode->pPlane);
    int side = (dot >= 0.0f) ? 0 : 1;

    // Front side
    recursiveWorldNodes(pNode->children[side]);

    for (int i = 0; i < pNode->iNumSurfaces; i++) {
        LevelSurface &surface = getLevelVars().baseSurfaces[(size_t)pNode->iFirstSurface + i];

        if (cullSurface(surface)) {
            continue;
        }

        getFrameVars().worldSurfacesToDraw.push_back((size_t)pNode->iFirstSurface + i);
    }

    // Back side
    recursiveWorldNodes(pNode->children[!side]);
}
