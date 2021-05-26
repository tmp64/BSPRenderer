#include <renderer/utils.h>
#include "world_state.h"
#include "renderer.h"

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int boxOnPlaneSide(const glm::vec3 &emins, const glm::vec3 &emaxs, const bsp::BSPPlane *p) {
    float dist1, dist2;
    int sides = 0;
    uint8_t signbits = signbitsForPlane(p->vNormal);

    // general case
    switch (signbits) {
    case 0:
        dist1 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emaxs[2];
        dist2 = p->vNormal[0] * emins[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emins[2];
        break;
    case 1:
        dist1 = p->vNormal[0] * emins[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emaxs[2];
        dist2 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emins[2];
        break;
    case 2:
        dist1 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emaxs[2];
        dist2 = p->vNormal[0] * emins[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emins[2];
        break;
    case 3:
        dist1 = p->vNormal[0] * emins[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emaxs[2];
        dist2 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emins[2];
        break;
    case 4:
        dist1 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emins[2];
        dist2 = p->vNormal[0] * emins[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emaxs[2];
        break;
    case 5:
        dist1 = p->vNormal[0] * emins[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emins[2];
        dist2 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emaxs[2];
        break;
    case 6:
        dist1 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emins[2];
        dist2 = p->vNormal[0] * emins[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emaxs[2];
        break;
    case 7:
        dist1 = p->vNormal[0] * emins[0] + p->vNormal[1] * emins[1] + p->vNormal[2] * emins[2];
        dist2 = p->vNormal[0] * emaxs[0] + p->vNormal[1] * emaxs[1] + p->vNormal[2] * emaxs[2];
        break;
    default:
        // shut up compiler
        dist1 = dist2 = 0;
        break;
    }

    if (dist1 >= p->fDist)
        sides = 1;
    if (dist2 < p->fDist)
        sides |= 2;

    return sides;
}

WorldState::WorldState() {
    AFW_ASSERT(!m_spInstance);
    m_spInstance = this;
}

WorldState::~WorldState() {
    AFW_ASSERT(m_spInstance == this);
    m_spInstance = nullptr;
}

void WorldState::loadLevel(const bsp::Level &level) {
    AFW_ASSERT(!m_pLevel);
    if (m_pLevel) {
        throw std::logic_error("world already loaded");
    }

    m_pLevel = &level;

    loadBrushModels();
    loadEntities();
}

BrushModel *WorldState::getBrushModel(size_t idx) {
    if (idx < m_BrushModels.size()) {
        return &m_BrushModels[idx];
    } else {
        return nullptr;
    }
}

void WorldState::onRendererReady() {
    for (auto &i : m_BrushModels) {
        // Optimize the model
        Renderer::get().optimizeBrushModel(&i);
    }
}

void WorldState::loadBrushModels() {
    auto &models = m_pLevel->getModels();
    auto &faces = m_pLevel->getFaces();
    m_BrushModels.resize(models.size());

    for (size_t i = 0; i < models.size(); i++) {
        auto &levelModel = models[i];
        auto &model = m_BrushModels[i];

        model.m_vMins = levelModel.nMins;
        model.m_vMaxs = levelModel.nMaxs;
        model.m_vOrigin = levelModel.vOrigin;
        std::copy(levelModel.iHeadnodes, levelModel.iHeadnodes + std::size(levelModel.iHeadnodes), model.m_iHeadnodes);
        model.m_nVisLeafs = levelModel.nVisLeafs;
        model.m_iFirstFace = levelModel.iFirstFace;
        model.m_iFaceNum = levelModel.nFaces;

        if (model.m_iFirstFace + model.m_iFaceNum > faces.size()) {
            throw std::runtime_error(fmt::format("model {}: invalid faces [{};{})", i, model.m_iFirstFace,
                                                 model.m_iFirstFace + model.m_iFaceNum));
        }

        // Calculate radius
        float radius = 0;
        auto &lvlSurfEdges = m_pLevel->getSurfEdges();
        auto &lvlVertices = m_pLevel->getVertices();

        for (int faceidx = model.m_iFirstFace; faceidx < model.m_iFirstFace + model.m_iFaceNum; faceidx++) {
            const bsp::BSPFace &face = m_pLevel->getFaces().at(faceidx);
            for (int j = 0; j < face.nEdges; j++) {
                glm::vec3 vertex;
                bsp::BSPSurfEdge iEdgeIdx = lvlSurfEdges.at((size_t)face.iFirstEdge + j);

                if (iEdgeIdx > 0) {
                    const bsp::BSPEdge &edge = m_pLevel->getEdges().at(iEdgeIdx);
                    vertex = lvlVertices.at(edge.iVertex[0]);
                } else {
                    const bsp::BSPEdge &edge = m_pLevel->getEdges().at(-iEdgeIdx);
                    vertex = lvlVertices.at(edge.iVertex[1]);
                }

                radius = std::max(radius, std::abs(vertex.x));
                radius = std::max(radius, std::abs(vertex.y));
                radius = std::max(radius, std::abs(vertex.z));
            }

        }

        model.m_flRadius = radius;
    }
}

void WorldState::loadEntities() {
    auto ents = m_pLevel->getEntities();

    for (size_t i = 0; i < ents.size(); i++) {
        std::unique_ptr<BaseEntity> pEnt = std::make_unique<BaseEntity>();
        try {
            pEnt->loadKeyValues(ents[i], (int)i);
            m_EntityList.push_back(std::move(pEnt));
        } catch (const std::exception &e) {
            logError("Entity {}: {}", i, e.what());
        }
    }

    logInfo("Loaded {} entities.", m_EntityList.size());
    
    size_t fail = ents.size() - m_EntityList.size();

    if (fail > 0) {
        logWarn("{} entities failed to load", fail);
    }
}

bool WorldState::boxInPvs(const glm::vec3 &origin, const glm::vec3 &mins, const glm::vec3 &maxs) {
    int leaf = m_pLevel->pointInLeaf(origin);
    const uint8_t *vis = m_pLevel->leafPVS(leaf, m_VisBuf);

    return isBoxVisible(mins, maxs, vis);
}

bool WorldState::isBoxVisible(const glm::vec3 &mins, const glm::vec3 &maxs, const uint8_t *visbits) {
    short leafList[MAX_BOX_LEAFS];

    if (!visbits) {
        return true;
    }

    int count = boxLeafNums(mins, maxs, appfw::span(leafList, MAX_BOX_LEAFS));

    for (int i = 0; i < count; i++) {
        int leafnum = leafList[i];

        if (leafnum != -1 && visbits[leafnum >> 3] & (1 << (leafnum & 7)))
            return true;
    }
    return false;
}

int WorldState::boxLeafNums(const glm::vec3 &mins, const glm::vec3 &maxs, appfw::span<short> list) {
    LeafList ll;
    ll.mins = mins;
    ll.maxs = maxs;
    ll.count = 0;
    ll.maxcount = (int)list.size();
    ll.list = list.data();
    ll.topnode = -1;
    ll.overflowed = false;

    boxLeafNums_r(ll, 0);

    return ll.count;
}

void WorldState::boxLeafNums_r(LeafList &ll, int node) {
    for (;;) {
        if (node < 0) {
            int leafidx = ~node;
            const bsp::BSPLeaf *pLeaf = &m_pLevel->getLeaves()[leafidx];

            if (pLeaf->nContents == bsp::CONTENTS_SOLID)
                return;

            // it's a leaf!
            if (ll.count >= ll.maxcount) {
                ll.overflowed = true;
                return;
            }

            ll.list[ll.count] = (short)(leafidx - 1);
            ll.count++;
            return;
        }

        const bsp::BSPNode *pNode = &m_pLevel->getNodes()[node];
        const bsp::BSPPlane &plane = m_pLevel->getPlanes()[pNode->iPlane];
        int s = boxOnPlaneSide(ll.mins, ll.maxs, &plane);

        if (s == 1) {
            node = pNode->iChildren[0];
        } else if (s == 2) {
            node = pNode->iChildren[1];
        } else {
            // go down both
            if (ll.topnode == -1)
                ll.topnode = node;
            boxLeafNums_r(ll, pNode->iChildren[0]);
            node = pNode->iChildren[1];
        }
    }
}
