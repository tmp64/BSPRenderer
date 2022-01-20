#include "world_state.h"
#include "main_view_renderer.h"
#include "bspviewer.h"

#include "player_start_entity.h"
#include "trigger_entity.h"

WorldState::WorldState(LevelAssetRef level) {
    AFW_ASSERT(!m_spInstance);
    m_spInstance = this;

    m_pLevelAsset = std::move(level);
    m_pLevel = &m_pLevelAsset->getLevel();

    loadBrushModels();
    loadEntities();
    optimizeBrushModels();
    m_Vis.setLevel(m_pLevel);
    m_MaterialLoader.init(BSPViewer::get().getMapName(), "assets:sound/materials.txt",
                          "assets:materials");
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

BaseEntity *WorldState::createEntity(std::string_view className, bsp::EntityKeyValues *kv) {
    std::unique_ptr<BaseEntity> pUniqueEnt;


    if (className == "info_player_start"
        || className == "info_player_deathmatch"
        || className == "info_player_coop") {
        pUniqueEnt = std::make_unique<PlayerStartEntity>();
    } else if (className == "func_ladder") {
        pUniqueEnt = std::make_unique<TriggerEntity>();
    } else if (className.substr(0, 8) == "trigger_") {
        pUniqueEnt = std::make_unique<TriggerEntity>();
    } else {
        pUniqueEnt = std::make_unique<BaseEntity>();
    }

    // Allocate idx
    int idx = (int)m_EntityList.size();
    m_EntityList.push_back(std::move(pUniqueEnt));

    // Spawn the entity
    BaseEntity *pEnt = m_EntityList[idx].get();
    pEnt->initialize(idx, className, kv);
    pEnt->spawn();
    return pEnt;
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
    m_LevelEntityDict.loadFromString(m_pLevel->getEntitiesLump());

    for (int i = 0; i < m_LevelEntityDict.size(); i++) {
        bsp::EntityKeyValues &kv = m_LevelEntityDict[i];
        createEntity(kv.getClassName(), &kv);
    }

    printi("Loaded {} entities.", m_EntityList.size());
    
    size_t fail = m_LevelEntityDict.size() - m_EntityList.size();

    if (fail > 0) {
        printw("{} entities failed to load", fail);
    }
}

void WorldState::optimizeBrushModels() {
    for (auto &i : m_BrushModels) {
        MainViewRenderer::get().optimizeBrushModel(&i);
    }
}
