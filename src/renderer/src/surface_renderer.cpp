#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <renderer/surface_renderer.h>
#include <renderer/utils.h>


ConVar<bool> r_lockpvs("r_lockpvs", false, "Lock current PVS to let devs see where it ends");
ConVar<bool> r_novis("r_novis", false, "Ignore visibility data");
ConVar<bool> r_lockcull("r_lockcull", false, "Disables updates to frustum culling");
ConVar<bool> r_no_frustum_culling("r_no_frustum_culling", false, "Disable frustum culling");

// Filled with 0xFF - all leaves are visible
static uint8_t s_NoVis[bsp::MAX_MAP_LEAFS / 8];

//----------------------------------------------------------------
// Context
//----------------------------------------------------------------
void SurfaceRenderer::Context::setPerspective(float fov, float aspect, float zNear, float zFar) {
    m_Type = ProjType::Perspective;

    float fov_x = fov;
    float fov_x_tan = 0;

    if (fov_x < 1.0) {
        fov_x_tan = 1.0;
    } else if (fov_x <= 179.0f) {
        fov_x_tan = tan(fov_x * glm::pi<float>() / 360.f);
    } else {
        fov_x_tan = 1.0;
    }

    float fov_y = atan(fov_x_tan / aspect) * 360.f / glm::pi<float>();
    float fov_y_tan = tan(fov_y * glm::pi<float>() / 360.f) * 4.f;

    float xMax = fov_y_tan * aspect;
    float xMin = -xMax;

    m_ProjMat = glm::frustum(xMin, xMax, -fov_y_tan, fov_y_tan, zNear, zFar);

    m_flHorFov = fov_x;
    m_flVertFov = fov_y;
    m_flAspect = aspect;
    m_flNearZ = zNear;
    m_flFarZ = zFar;
}

void SurfaceRenderer::Context::setPerspViewOrigin(const glm::vec3 &origin, const glm::vec3 &angles) {
    m_Type = ProjType::Perspective;
    m_vViewOrigin = origin;
    m_vViewAngles = angles;

    m_ViewMat = glm::identity<glm::mat4>();
    m_ViewMat = glm::rotate(m_ViewMat, glm::radians(-90.f), {1.0f, 0.0f, 0.0f});
    m_ViewMat = glm::rotate(m_ViewMat, glm::radians(90.f), {0.0f, 0.0f, 1.0f});
    m_ViewMat = glm::rotate(m_ViewMat, glm::radians(-m_vViewAngles.z), {1.0f, 0.0f, 0.0f});
    m_ViewMat = glm::rotate(m_ViewMat, glm::radians(-m_vViewAngles.x), {0.0f, 1.0f, 0.0f});
    m_ViewMat = glm::rotate(m_ViewMat, glm::radians(-m_vViewAngles.y), {0.0f, 0.0f, 1.0f});
    m_ViewMat = glm::translate(m_ViewMat, {-m_vViewOrigin.x, -m_vViewOrigin.y, -m_vViewOrigin.z});
}

void SurfaceRenderer::Context::setupFrustum() {
    if (r_lockcull.getValue()) {
        // Don't update
        return;
    }

    // Build the transformation matrix for the given view angles
    angleVectors(m_vViewAngles, &m_vForward, &m_vRight, &m_vUp);

    // Setup frustum
    // rotate m_vForward right by FOV_X/2 degrees
    m_Frustum[0].vNormal = rotatePointAroundVector(m_vUp, m_vForward, -(90 - m_flHorFov / 2));
    // rotate m_vForward left by FOV_X/2 degrees
    m_Frustum[1].vNormal = rotatePointAroundVector(m_vUp, m_vForward, 90 - m_flHorFov / 2);
    // rotate m_vForward up by FOV_Y/2 degrees
    m_Frustum[2].vNormal = rotatePointAroundVector(m_vRight, m_vForward, 90 - m_flVertFov / 2);
    // rotate m_vForward down by FOV_Y/2 degrees
    m_Frustum[3].vNormal = rotatePointAroundVector(m_vRight, m_vForward, -(90 - m_flVertFov / 2));
    // near clipping plane
    m_Frustum[4].vNormal = m_vForward;

    for (size_t i = 0; i < 5; i++) {
        m_Frustum[i].nType = bsp::PlaneType::PlaneNonAxial;
        m_Frustum[i].fDist = glm::dot(m_vViewOrigin, m_Frustum[i].vNormal);
        m_Frustum[i].signbits = signbitsForPlane(m_Frustum[i].vNormal);
    }

    // Far clipping plane
    glm::vec3 farPoint = vectorMA(m_vViewOrigin, m_flFarZ, m_vForward);
    m_Frustum[5].vNormal = -m_vForward;
    m_Frustum[5].nType = bsp::PlaneType::PlaneNonAxial;
    m_Frustum[5].fDist = glm::dot(farPoint, m_Frustum[5].vNormal);
    m_Frustum[5].signbits = signbitsForPlane(m_Frustum[5].vNormal);
}

//----------------------------------------------------------------
// SurfaceRenderer
//----------------------------------------------------------------
SurfaceRenderer::SurfaceRenderer() {
    memset(s_NoVis, 0xFF, sizeof(s_NoVis));
}

void SurfaceRenderer::setLevel(bsp::Level *level) {
    if (m_pLevel == level) {
        return;
    }

    // Reset level data
    m_Data = LevelData();
    m_pLevel = level;
    
    if (level) {
        try {
            createSurfaces();
            createLeaves();
            createNodes();
        } catch (...) {
            m_pLevel = nullptr;
            m_Data = LevelData();
            throw;
        }
    }
}

void SurfaceRenderer::calcWorldSurfaces() {
    AFW_ASSERT(m_pContext);
    m_pContext->getWorldSurfaces().clear();
    m_pContext->getWorldSurfaces().reserve(m_Data.surfaces.size());
    m_pContext->getSkySurfaces().clear();

    if (!m_pLevel) {
        return;
    }

    AFW_ASSERT(m_pContext->m_iVisFrame != 0 || m_pContext->m_NodeVisFrame.empty());
    AFW_ASSERT(m_pContext->m_iVisFrame != 0 || m_pContext->m_LeafVisFrame.empty());

    if (m_pContext->m_iVisFrame == 0) {
        m_pContext->m_NodeVisFrame.resize(m_Data.nodes.size());
        m_pContext->m_LeafVisFrame.resize(m_Data.leaves.size());
        m_pContext->m_iVisFrame++;
    }

    m_pContext->setupFrustum();
    markLeaves();
    recursiveWorldNodes(0);
}

void SurfaceRenderer::createSurfaces() {
    auto &lvlFaces = m_pLevel->getFaces();
    auto &lvlSurfEdges = m_pLevel->getSurfEdges();
    m_Data.surfaces.resize(lvlFaces.size());

    for (size_t i = 0; i < lvlFaces.size(); i++) {
        const bsp::BSPFace &face = lvlFaces[i];
        Surface &surface = m_Data.surfaces[i];

        if (face.iFirstEdge + face.nEdges > (uint32_t)lvlSurfEdges.size()) {
            logWarn("SurfaceRenderer::createSurfaces: Bad surface {}", i);
            continue;
        }

        surface.iFirstEdge = face.iFirstEdge;
        surface.iNumEdges = face.nEdges;
        surface.pPlane = &m_pLevel->getPlanes().at(face.iPlane);

        if (face.nPlaneSide) {
            surface.iFlags |= SURF_PLANEBACK;
        }

        // Create vertices and calculate bounds
        surface.vMins[0] = surface.vMins[1] = surface.vMins[2] = 999999.0f;
        surface.vMaxs[0] = surface.vMaxs[1] = surface.vMaxs[2] = -999999.0f;
        auto &lvlVertices = m_pLevel->getVertices();

        for (int j = 0; j < surface.iNumEdges; j++) {
            if (j == MAX_SIDE_VERTS) {
                logWarn("createBaseSurfaces(): polygon {} is too large (exceeded {} vertices)", j, MAX_SIDE_VERTS);
                break;
            }

            glm::vec3 vertex;
            bsp::BSPSurfEdge iEdgeIdx = lvlSurfEdges.at((size_t)surface.iFirstEdge + j);

            if (iEdgeIdx > 0) {
                const bsp::BSPEdge &edge = m_pLevel->getEdges().at(iEdgeIdx);
                vertex = lvlVertices.at(edge.iVertex[0]);
            } else {
                const bsp::BSPEdge &edge = m_pLevel->getEdges().at(-iEdgeIdx);
                vertex = lvlVertices.at(edge.iVertex[1]);
            }

            surface.vVertices.push_back(vertex);

            // Add vertex to bounds
            for (int k = 0; k < 3; k++) {
                float val = vertex[k];

                if (val < surface.vMins[k])
                    surface.vMins[k] = val;

                if (val > surface.vMaxs[k])
                    surface.vMaxs[k] = val;
            }
        }

        surface.vVertices.shrink_to_fit();

        // Find material
        const bsp::BSPTextureInfo &texInfo = m_pLevel->getTexInfo().at(face.iTextureInfo);
        const bsp::BSPMipTex &tex = m_pLevel->getTextures().at(texInfo.iMiptex);
        surface.pTexInfo = &texInfo;
        surface.nMatIndex = MaterialManager::get().findMaterial(tex.szName);

        if (!strncmp(tex.szName, "sky", 3)) {
            surface.iFlags |= SURF_DRAWSKY;
        }
    }
}

void SurfaceRenderer::createLeaves() {
    auto &lvlLeaves = getLevel()->getLeaves();
    m_Data.leaves.resize(lvlLeaves.size());

    for (size_t i = 0; i < lvlLeaves.size(); i++) {
        const bsp::BSPLeaf &lvlLeaf = lvlLeaves[i];
        Leaf &leaf = m_Data.leaves[i];

        leaf.nContents = lvlLeaf.nContents;

        if (lvlLeaf.nVisOffset != -1 && !getLevel()->getVisData().empty()) {
            leaf.pCompressedVis = &getLevel()->getVisData().at(lvlLeaf.nVisOffset);
        }
    }
}

void SurfaceRenderer::createNodes() {
    auto &lvlNodes = m_pLevel->getNodes();
    m_Data.nodes.resize(lvlNodes.size());

    for (size_t i = 0; i < lvlNodes.size(); i++) {
        const bsp::BSPNode &lvlNode = lvlNodes[i];
        Node &node = m_Data.nodes[i];
        
        node.iFirstSurface = lvlNode.firstFace;
        node.iNumSurfaces = lvlNode.nFaces;
        node.pPlane = &m_pLevel->getPlanes().at(lvlNode.iPlane);
        node.iChildren[0] = lvlNode.iChildren[0];
        node.iChildren[1] = lvlNode.iChildren[1];
    }

    updateNodeParents(0, 0);
}

void SurfaceRenderer::updateNodeParents(int iNode, int parent) {
    if (iNode >= 0) {
        Node &node = m_Data.nodes[iNode];
        node.iParentIdx = parent;
        updateNodeParents(node.iChildren[0], iNode);
        updateNodeParents(node.iChildren[1], iNode);
    } else {
        Leaf &leaf = m_Data.leaves[~iNode];
        leaf.iParentIdx = parent;
    }
}

void SurfaceRenderer::markLeaves() noexcept {
    if (r_lockpvs.getValue()) {
        // PVS update is disabled
        return;
    }
    
    int viewleaf = pointInLeaf(m_pContext->m_vViewOrigin);

    if (viewleaf == m_pContext->m_iViewLeaf) {
        // Leaf not changed, keep PVS as is
        return;
    }

    m_pContext->m_iViewLeaf = viewleaf;
    m_pContext->m_iVisFrame++;

    uint8_t *vis = nullptr;
    uint8_t solid[4096];

    if (r_novis.getValue()) {
        AFW_ASSERT(((m_Data.leaves.size() + 7) >> 3) <= sizeof(solid));
        vis = solid;
        memset(solid, 0xFF, (m_Data.leaves.size() + 7) >> 3);
    } else {
        vis = leafPVS(viewleaf);
    }

    // Node 0 is always visible
    m_pContext->m_NodeVisFrame[0] = m_pContext->m_iVisFrame;

    for (unsigned i = 0; i < m_Data.leaves.size() - 1; i++) {
        if (vis[i >> 3] & (1 << (i & 7))) {
            int node = ~(i + 1);

            do {
                if (node < 0) {
                    // Leaf
                    if (m_pContext->m_LeafVisFrame[~node] == m_pContext->m_iVisFrame) {
                        break;
                    }

                    m_pContext->m_LeafVisFrame[~node] = m_pContext->m_iVisFrame;
                    node = m_Data.leaves[~node].iParentIdx;
                } else {
                    // Node
                    if (m_pContext->m_NodeVisFrame[node] == m_pContext->m_iVisFrame) {
                        break;
                    }

                    m_pContext->m_NodeVisFrame[node] = m_pContext->m_iVisFrame;
                    node = m_Data.nodes[node].iParentIdx;
                }
            } while (node);
        }
    }
}

void SurfaceRenderer::recursiveWorldNodes(int node) noexcept {
    if (node < 0) {
        // Leaf
        return;
    }

    if (m_pContext->m_NodeVisFrame[node] != m_pContext->m_iVisFrame) {
        // Not in PVS
        return;
    }

    Node *pNode = &m_Data.nodes[node];

    float dot = planeDiff(m_pContext->m_vViewOrigin, *pNode->pPlane);
    int side = (dot >= 0.0f) ? 0 : 1;

    // Front side
    recursiveWorldNodes(pNode->iChildren[side]);

    // Draw surfaces of current node
    for (unsigned i = 0; i < pNode->iNumSurfaces; i++) {
        unsigned idx = pNode->iFirstSurface + i;
        Surface &surf = m_Data.surfaces[idx];

        if (cullSurface(surf)) {
            continue;
        }

        if (surf.iFlags & SURF_DRAWSKY) {
            m_pContext->m_SkySurfaces.push_back(idx);
        } else {
            m_pContext->m_WorldSurfaces.push_back(idx);
        }
    }

    // Back side
    recursiveWorldNodes(pNode->iChildren[!side]);
}

bool SurfaceRenderer::cullSurface(Surface &surface) noexcept {
    if (m_pContext->m_Cull == Cull::None) {
        return false;
    }

    float dist = planeDiff(m_pContext->m_vViewOrigin, *surface.pPlane);

    if (m_pContext->m_Cull == Cull::Back) {
        // Back face culling
        if (surface.iFlags & SURF_PLANEBACK) {
            if (dist >= -BACKFACE_EPSILON)
                return true; // wrong side
        } else {
            if (dist <= BACKFACE_EPSILON)
                return true; // wrong side
        }
    } else if (m_pContext->m_Cull == Cull::Front) {
        // Front face culling
        if (surface.iFlags & SURF_PLANEBACK) {
            if (dist <= BACKFACE_EPSILON)
                return true; // wrong side
        } else {
            if (dist >= -BACKFACE_EPSILON)
                return true; // wrong side
        }
    }

    // Frustum culling
    return cullBox(surface.vMins, surface.vMaxs);
}

bool SurfaceRenderer::cullBox(glm::vec3 mins, glm::vec3 maxs) noexcept {
    if (m_pContext->m_Cull == Cull::None || r_no_frustum_culling.getValue()) {
        return false;
    }

    unsigned i;
    const Plane *p;

    for (i = (unsigned)m_pContext->m_Frustum.size(), p = &m_pContext->m_Frustum[0]; i > 0; i--, p++) {
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

int SurfaceRenderer::pointInLeaf(glm::vec3 p) noexcept {
    int node = 0;

    for (;;) {
        if (node < 0) {
            return node;
        }

        Node *pNode = &m_Data.nodes[node];
        const bsp::BSPPlane &plane = *pNode->pPlane;
        float d = glm::dot(p, plane.vNormal) - plane.fDist;

        if (d > 0) {
            node = pNode->iChildren[0];
        } else {
            node = pNode->iChildren[1];
        }
    }
}

uint8_t *SurfaceRenderer::leafPVS(int leaf) noexcept {
    AFW_ASSERT(leaf < 0);
    int leafIdx = ~leaf;

    if (leafIdx == 0) {
        return s_NoVis;
    }

    Leaf *pLeaf = &m_Data.leaves[leafIdx];

    // Decompress VIS
    int row = ((int)m_Data.leaves.size() + 7) >> 3;
    const uint8_t *in = pLeaf->pCompressedVis;
    uint8_t *out = m_pContext->m_DecompressedVis.data();

    if (!in) {
        // Nno vis info, so make all visible
        while (row) {
            *out++ = 0xff;
            row--;
        }
        return m_pContext->m_DecompressedVis.data();
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
    } while (out - m_pContext->m_DecompressedVis.data() < row);

    return m_pContext->m_DecompressedVis.data();
}
