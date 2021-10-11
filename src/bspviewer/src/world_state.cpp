#include "world_state.h"
#include "main_view_renderer.h"

WorldState::WorldState(LevelAssetRef level) {
    AFW_ASSERT(!m_spInstance);
    m_spInstance = this;

    m_pLevelAsset = std::move(level);
    m_pLevel = &m_pLevelAsset->getLevel();

    loadBrushModels();
    loadEntities();
    optimizeBrushModels();
    m_Vis.setLevel(m_pLevel);
}

WorldState::~WorldState() {
    AFW_ASSERT(m_spInstance == this);
    m_spInstance = nullptr;
    m_Vis.setLevel(nullptr);
    MainViewRenderer::get().unloadLevel();
}

BrushModel *WorldState::getBrushModel(size_t idx) {
    if (idx < m_BrushModels.size()) {
        return &m_BrushModels[idx];
    } else {
        return nullptr;
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
            printe("Entity {}: {}", i, e.what());
        }
    }

    printi("Loaded {} entities.", m_EntityList.size());
    
    size_t fail = ents.size() - m_EntityList.size();

    if (fail > 0) {
        printw("{} entities failed to load", fail);
    }
}

void WorldState::optimizeBrushModels() {
    for (auto &i : m_BrushModels) {
        MainViewRenderer::get().optimizeBrushModel(&i);
    }
}
