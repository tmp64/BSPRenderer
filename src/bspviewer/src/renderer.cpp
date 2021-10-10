#include "renderer.h"
#include "world_state.h"
#include "bspviewer.h"
#include "assets/asset_manager.h"

static ConVar<float> fov("fov", 120.f, "Horizontal field of view");

class EntityBoxShader : public BaseShader {
public:
    EntityBoxShader() {
        setTitle("EntityBoxShader");
        setVert("assets:shaders/entity_box.vert");
        setFrag("assets:shaders/entity_box.frag");

        addUniform(m_uGlobalUniform, "GlobalUniform", SceneRenderer::GLOBAL_UNIFORM_BIND);
    }

private:
    UniformBlock m_uGlobalUniform;
};

static EntityBoxShader s_EntityBoxShader;

Renderer::Renderer() {
    AFW_ASSERT(!m_spInstance);
    m_spInstance = this;

    m_SceneRenderer.setEngine(this);
    m_VisEnts.resize(MAX_VISIBLE_ENTS);

    // Imported from blender
    glm::vec3 bv[] = {{0.500000f, 0.500000f, -0.500000},  {0.500000f, -0.500000f, -0.500000},
                      {0.500000f, 0.500000f, 0.500000},   {0.500000f, -0.500000f, 0.500000},
                      {-0.500000f, 0.500000f, -0.500000}, {-0.500000f, -0.500000f, -0.500000},
                      {-0.500000f, 0.500000f, 0.500000},  {-0.500000f, -0.500000f, 0.500000}};

    glm::vec3 bn[] = {{0.0000f, 1.0000f, 0.0000},  {0.0000f, 0.0000f, 1.0000},
                      {-1.0000f, 0.0000f, 0.0000}, {0.0000f, -1.0000f, 0.0000},
                      {1.0000f, 0.0000f, 0.0000},  {0.0000f, 0.0000f, -1.0000}};

    glm::vec3 faceVerts[] = {
        bv[0], bn[0], bv[2], bn[0], bv[4], bn[0], bv[3], bn[1], bv[7], bn[1], bv[2], bn[1],
        bv[7], bn[2], bv[5], bn[2], bv[6], bn[2], bv[5], bn[3], bv[7], bn[3], bv[1], bn[3],
        bv[1], bn[4], bv[3], bn[4], bv[0], bn[4], bv[5], bn[5], bv[1], bn[5], bv[4], bn[5],
        bv[2], bn[0], bv[6], bn[0], bv[4], bn[0], bv[7], bn[1], bv[6], bn[1], bv[2], bn[1],
        bv[5], bn[2], bv[4], bn[2], bv[6], bn[2], bv[7], bn[3], bv[3], bn[3], bv[1], bn[3],
        bv[3], bn[4], bv[2], bn[4], bv[0], bn[4], bv[1], bn[5], bv[0], bn[5], bv[4], bn[5]};

    m_BoxVbo.create("Entity Box VBO");
    m_BoxVbo.bind(GL_ARRAY_BUFFER);
    m_BoxVbo.bufferData(GL_ARRAY_BUFFER, sizeof(faceVerts), &faceVerts, GL_STATIC_DRAW);

    m_BoxInstances.create("Entity Box Instances");
    m_BoxInstances.bind(GL_ARRAY_BUFFER);
    m_BoxInstances.bufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * MAX_VISIBLE_ENTS, nullptr, GL_DYNAMIC_DRAW);

    m_BoxVao.create();
    glBindVertexArray(m_BoxVao);
    m_BoxVbo.bind(GL_ARRAY_BUFFER);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3), (void *)0); // position
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3),
                          (void *)sizeof(glm::vec3)); // normal

    // transformations attribute
    m_BoxInstances.bind(GL_ARRAY_BUFFER);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(glm::mat4),
                          (void *)(0 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(glm::mat4),
                          (void *)(1 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, false, sizeof(glm::mat4),
                          (void *)(2 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, false, sizeof(glm::mat4),
                          (void *)(3 * sizeof(glm::vec4)));
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);

    m_BoxTransforms.resize(MAX_VISIBLE_ENTS);
    glBindVertexArray(0);
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
    updateVisibleEnts();
    m_SceneRenderer.renderScene(0, 0, 0); // 0 is the main framebuffer
}

void Renderer::optimizeBrushModel(Model *model) {
    m_SceneRenderer.optimizeBrushModel(model);
}

Material *Renderer::getMaterial(const bsp::BSPMipTex &tex) {
    WADMaterialAssetRef mat = AssetManager::get().findMaterialByName(tex.szName);
    if (mat) {
        return mat->getMaterial();
    } else {
        return nullptr;
    }
}

void Renderer::drawNormalTriangles(unsigned &drawcallCount) {
    if (m_uBoxCount > 0) {
        glBindVertexArray(m_BoxVao);
        s_EntityBoxShader.enable();
        glDrawArraysInstanced(GL_TRIANGLES, 0, BOX_VERT_COUNT, m_uBoxCount);
        drawcallCount++;
        s_EntityBoxShader.disable();
        glBindVertexArray(0);
    }
}

void Renderer::drawTransTriangles(unsigned &drawcallCount) {}

void Renderer::updateVisibleEnts() {
    appfw::Prof prof("Entity List");
    m_SceneRenderer.clearEntities();
    auto &ents = WorldState::get().getEntList();
    unsigned entCount = 0;
    m_uBoxCount = 0;

    for (size_t i = 0; i < ents.size(); i++) {
        BaseEntity *pEnt = ents[i].get();

        if (!pEnt->getModel()) {
            // Draw a box for this model
            if (m_uBoxCount == MAX_VISIBLE_ENTS) {
                continue;
            }

            glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(16.0f));
            glm::mat4 translate = glm::translate(glm::identity<glm::mat4>(), pEnt->getOrigin());
            m_BoxTransforms[m_uBoxCount] = translate * scale;
            m_uBoxCount++;
        } else if (pEnt->getModel() && pEnt->getModel()->getType() == ModelType::Brush) {
            if (entCount == MAX_VISIBLE_ENTS) {
                continue;
            }

            // Check if visible in PVS
            {
                glm::vec3 mins = pEnt->getOrigin() + pEnt->getModel()->getMins();
                glm::vec3 maxs = pEnt->getOrigin() + pEnt->getModel()->getMaxs();
                if (!Vis::get().boxInPvs(BSPViewer::get().getCameraPos(), mins, maxs)) {
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
        }
    }

    // Update box buffer
    if (m_uBoxCount > 0) {
        m_BoxInstances.bind(GL_ARRAY_BUFFER);
        m_BoxInstances.bufferSubData(GL_ARRAY_BUFFER, 0, m_uBoxCount * sizeof(glm::mat4),
                                     m_BoxTransforms.data());
    }
}
