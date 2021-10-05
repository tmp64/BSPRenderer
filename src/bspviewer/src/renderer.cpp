#include "renderer.h"
#include "world_state.h"
#include "bspviewer.h"
#include "assets/asset_manager.h"

static ConVar<float> fov("fov", 120.f, "Horizontal field of view");

Renderer::Renderer() {
    AFW_ASSERT(!m_spInstance);
    m_spInstance = this;

    m_SceneRenderer.setMaterialCallback([](const bsp::BSPMipTex &tex) -> Material * {
        WADMaterialAssetRef mat = AssetManager::get().findMaterialByName(tex.szName);
        if (mat) {
            return mat->getMaterial();
        } else {
            return nullptr;
        }
    });
    m_SceneRenderer.setEntListCallback([this]() { addVisibleEnts(); });
    m_VisEnts.resize(MAX_VISIBLE_ENTS);
}

Renderer::~Renderer() {
    AFW_ASSERT(m_spInstance == this);
    m_spInstance = nullptr;
}

void Renderer::setViewportSize(const glm::ivec2 &size) { m_SceneRenderer.setViewportSize(size); }

void Renderer::loadLevel(LevelAssetRef &level) {
    AFW_ASSERT(m_SceneRenderer.getLevel() == nullptr);
    m_SceneRenderer.beginLoading(&level->getLevel(), level->getPath());
}

void Renderer::unloadLevel() {
    m_SceneRenderer.unloadLevel();
}

void Renderer::tick() {
    m_SceneRenderer.showDebugDialog("Renderer");
}

bool Renderer::loadingTick() {
    AFW_ASSERT(m_SceneRenderer.isLoading());
    return m_SceneRenderer.loadingTick();
}

void Renderer::draw() {
    m_SceneRenderer.setPerspective(fov.getValue(), BSPViewer::get().getAspectRatio(), 4, 8192);
    m_SceneRenderer.setPerspViewOrigin(BSPViewer::get().getCameraPos(), BSPViewer::get().getCameraRot());
    m_SceneRenderer.renderScene(0, 0, 0); // 0 is the main framebuffer
}

void Renderer::optimizeBrushModel(Model *model) { m_SceneRenderer.optimizeBrushModel(model); }

void Renderer::addVisibleEnts() {
    auto &ents = WorldState::get().getEntList();
    unsigned entCount = 0;

    for (size_t i = 0; i < ents.size(); i++) {
        BaseEntity *pEnt = ents[i].get();

        if (pEnt->getModel() && pEnt->getModel()->getType() == ModelType::Brush) {
            // Check if visible in PVS
            {
                glm::vec3 mins = pEnt->getOrigin() + pEnt->getModel()->getMins();
                glm::vec3 maxs = pEnt->getOrigin() + pEnt->getModel()->getMaxs();
                if (!WorldState::get().boxInPvs(BSPViewer::get().getCameraPos(), mins, maxs)) {
                    continue;
                }
            }


            ClientEntity *pClent = &m_VisEnts[entCount];
            pClent->m_vOrigin = pEnt->getOrigin();
            pClent->m_vAngles = pEnt->getAngles();
            pClent->m_iRenderMode = pEnt->getRenderMode();
            pClent->m_iRenderFx = pEnt->getRenderFx();
            pClent->m_iFxAmount = pEnt->getFxAmount();
            pClent->m_vFxColor = pEnt->getFxColor();
            pClent->m_pModel = pEnt->getModel();

            m_SceneRenderer.addEntity(pClent);
            entCount++;

            if (entCount == m_VisEnts.size()) {
                break;
            }
        }
    }
}
