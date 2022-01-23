#include "world_state.h"
#include "main_view_renderer.h"
#include "bspviewer.h"

#include "entities/light_entity.h"
#include "entities/player_start_entity.h"
#include "entities/trigger_entity.h"

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

    initLightStyles();
}

WorldState::~WorldState() {
    AFW_ASSERT(m_spInstance == this);
    m_spInstance = nullptr;
    m_Vis.setLevel(nullptr);
    MainViewRenderer::get().unloadLevel();
}

void WorldState::tick() {
    m_flTimeDelta = BSPViewer::get().getTimeDelta();
    m_flTime += m_flTimeDelta;

    animateLights();
}

BrushModel *WorldState::getBrushModel(size_t idx) {
    if (idx < m_BrushModels.size()) {
        return &m_BrushModels[idx];
    } else {
        return nullptr;
    }
}

BaseEntity *WorldState::getEntity(int entIdx) {
    AFW_ASSERT(entIdx >= 0);
    if (entIdx < m_EntityList.size()) {
        return m_EntityList[entIdx].get();
    } else {
        return nullptr;
    }
}

BaseEntity *WorldState::createEntity(std::string_view className, bsp::EntityKeyValues *kv) {
    std::unique_ptr<BaseEntity> pUniqueEnt;

    if (className == "light"
        || className == "light_spot"
        || className == "light_environment") {
        pUniqueEnt = std::make_unique<LightEntity>();
    } else if (className == "info_player_start"
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

void WorldState::setLightStyle(int style, const char *pattern, float animTime) {
    style = std::clamp(style, 0, MAX_LIGHTSTYLES - 1);
    LightStyle &ls = m_LightStyles[style];

    // Pattern string
    ls.patternLength = (int)std::min(strlen(pattern), sizeof(ls.pattern) - 1);
    memcpy(ls.pattern, pattern, ls.patternLength + 1);
    ls.pattern[sizeof(ls.pattern) - 1] = '\0';

    // Pattern floats
    for (int i = 0; i < ls.patternLength; i++) {
        ls.scale[i] = (pattern[i] - 'a') / 12.0f;
    }

    ls.startTime = m_flTime + animTime;
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

void WorldState::initLightStyles() {
    // 0 normal
    setLightStyle(0, "m");

    // 1 FLICKER (first variety)
    setLightStyle(1, "mmnmmommommnonmmonqnmmo");

    // 2 SLOW STRONG PULSE
    setLightStyle(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

    // 3 CANDLE (first variety)
    setLightStyle(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

    // 4 FAST STROBE
    setLightStyle(4, "mamamamamama");

    // 5 GENTLE PULSE 1
    setLightStyle(5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");

    // 6 FLICKER (second variety)
    setLightStyle(6, "nmonqnmomnmomomno");

    // 7 CANDLE (second variety)
    setLightStyle(7, "mmmaaaabcdefgmmmmaaaammmaamm");

    // 8 CANDLE (third variety)
    setLightStyle(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

    // 9 SLOW STROBE (fourth variety)
    setLightStyle(9, "aaaaaaaazzzzzzzz");

    // 10 FLUORESCENT FLICKER
    setLightStyle(10, "mmamammmmammamamaaamammma");

    // 11 SLOW PULSE NOT FADE TO BLACK
    setLightStyle(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

    // 12 UNDERWATER LIGHT MUTATION
    // this light only distorts the lightmap - no contribution
    // is made to the brightness of affected surfaces
    setLightStyle(12, "mmnnmmnnnmmnn");

    // styles 32-62 are assigned by the light program for switchable lights

    // 63 testing
    setLightStyle(63, "a");
}

void WorldState::animateLights() {
    for (int i = 0; i < MAX_LIGHTSTYLES; i++) {
        LightStyle &ls = m_LightStyles[i];
        float scale = 0;

        if (ls.patternLength == 0) {
            scale = 0;
        } else if (ls.patternLength == 1) {
            scale = ls.scale[0];
        } else {
            float time = m_flTime - ls.startTime;
            int frame = (int)(time * LIGHT_ANIM_FREQ);
            scale = ls.scale[frame % ls.patternLength];
        }

        MainViewRenderer::get().setLightstyleScale(i, scale);
    }
}
