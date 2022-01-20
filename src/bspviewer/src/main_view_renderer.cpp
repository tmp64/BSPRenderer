#include <appfw/str_utils.h>
#include <material_system/shader.h>
#include <renderer/utils.h>
#include <imgui_impl_shaders.h>
#include "main_view_renderer.h"
#include "world_state.h"
#include "bspviewer.h"
#include "assets/asset_manager.h"

extern ConVar<bool> r_drawworld;
extern ConVar<bool> r_drawents;

static ConVar<float> m_sens("m_sens", 0.15f, "Mouse sensitivity (degrees/pixel)");
static ConVar<float> cam_speed("cam_speed", 1000.f, "Camera speed");
static ConfigItem<float> v_fov("main_view_fov", 110.f, "Horizontal field of view");
static KeyBind grabToggleBind("Toggle main view mouse grab", KeyCode::Z);

static ConCommand cmd_getpos("getpos", "", [](const CmdString &) {
    auto pos = MainViewRenderer::get().getCameraPos();
    auto rot = MainViewRenderer::get().getCameraRot();
    printi("setpos {} {} {} {} {} {}", pos.x, pos.y, pos.z, rot.x, rot.y, rot.z);
});

static ConCommand cmd_setpos("setpos", "", [](const CmdString &cmd) {
    if (cmd.size() < 4) {
        printi("setpos <x> <y> <z> [pitch] [yaw] [roll]");
        return;
    }

    glm::vec3 pos = {std::stof(cmd[1]), std::stof(cmd[2]), std::stof(cmd[3])};
    MainViewRenderer::get().setCameraPos(pos);

    if (cmd.size() >= 7) {
        glm::vec3 rot = {std::stof(cmd[4]), std::stof(cmd[5]), std::stof(cmd[6])};
        MainViewRenderer::get().setCameraRot(rot);
    }
});

class MainViewShader : public ImGuiShader {
public:
    MainViewShader(unsigned type = 0)
        : ImGuiShader(type) {
        setVert("assets:shaders/imgui/gamma.vert");
        setFrag("assets:shaders/imgui/gamma.frag");
        setFramebufferSRGB(false);
    }

private:
    std::unique_ptr<Shader> createShaderInfoInstance(unsigned typeIdx) override {
        return std::make_unique<MainViewShader>(typeIdx);
    }
};

class EntityBoxShader : public ShaderT<EntityBoxShader> {
public:
    EntityBoxShader(unsigned type = 0)
        : BaseClass(type) {
        setTitle("EntityBoxShader");
        setVert("assets:shaders/entity_box.vert");
        setFrag("assets:shaders/entity_box.frag");
        setTypes(SHADER_TYPE_CUSTOM);

        addUniform(m_uGlobalUniform, "GlobalUniform", SceneRenderer::GLOBAL_UNIFORM_BIND);
    }

private:
    UniformBlock m_uGlobalUniform;
};

static MainViewShader s_MainViewShader;
static EntityBoxShader s_EntityBoxShader;

static ImVec2 imVecFloor(ImVec2 vec) {
    return ImVec2(floor(vec.x), floor(vec.y));
}

MainViewRenderer::MainViewRenderer() {
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

    m_BoxVbo.create(GL_ARRAY_BUFFER, "Entity Box VBO");
    m_BoxVbo.bind();
    m_BoxVbo.init(sizeof(faceVerts), &faceVerts, GL_STATIC_DRAW);

    m_BoxInstances.create(GL_ARRAY_BUFFER, "Entity Box Instances");
    m_BoxInstances.bind();
    m_BoxInstances.init(sizeof(BoxInstance) * MAX_VISIBLE_ENTS, nullptr, GL_DYNAMIC_DRAW);

    m_BoxVao.create();
    glBindVertexArray(m_BoxVao);
    m_BoxVbo.bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3), (void *)0); // position
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3),
                          (void *)sizeof(glm::vec3)); // normal


    // Colors and transformations attribute
    m_BoxInstances.bind();

    // Colors
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(BoxInstance),
                          (void *)(0 * sizeof(glm::vec4)));

    // Transforms
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(BoxInstance),
                          (void *)(1 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, false, sizeof(BoxInstance),
                          (void *)(2 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, false, sizeof(BoxInstance),
                          (void *)(3 * sizeof(glm::vec4)));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, false, sizeof(BoxInstance),
                          (void *)(4 * sizeof(glm::vec4)));
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);

    m_BoxInstancesData.resize(MAX_VISIBLE_ENTS);
    glBindVertexArray(0);

    m_pBoxMaterial = MaterialSystem::get().createMaterial("Entity Box");
    m_pBoxMaterial->setSize(1, 1);
    m_pBoxMaterial->setShader(SHADER_TYPE_CUSTOM_IDX, &s_EntityBoxShader);

    // Backbuffer
    m_pBackbufferMat = MaterialSystem::get().createMaterial("Main View");
    m_pBackbufferMat->setSize(1, 1);
    m_pBackbufferMat->setShader(SHADER_TYPE_IMGUI_IDX, &s_MainViewShader);
    m_pBackbufferMat->setShader(SHADER_TYPE_IMGUI_LINEAR_IDX, &s_MainViewShader);
    m_pBackbufferMat->setTexture(0, std::make_unique<Texture2D>());
}

MainViewRenderer::~MainViewRenderer() {
    AFW_ASSERT(m_spInstance == this);
    m_spInstance = nullptr;
    MaterialSystem::get().destroyMaterial(m_pBoxMaterial);
    m_pBoxMaterial = nullptr;

    m_Framebuffer.destroy();
    MaterialSystem::get().destroyMaterial(m_pBackbufferMat);
    m_pBackbufferMat = nullptr;
}

bool MainViewRenderer::isWorldRenderingEnabled() {
    return r_drawworld.getValue();
}

bool MainViewRenderer::isEntityRenderingEnabled() {
    return r_drawents.getValue();
}

void MainViewRenderer::setSurfaceTint(int surface, glm::vec4 color) {
    AFW_ASSERT(WorldState::get());
    m_SceneRenderer.setSurfaceTint(surface, color);
}

void MainViewRenderer::loadLevel(LevelAssetRef &level) {
    AFW_ASSERT(m_SceneRenderer.getLevel() == nullptr);
    m_SceneRenderer.beginLoading(&level->getLevel(), level->getPath());
}

void MainViewRenderer::unloadLevel() {
    if (InputSystem::get().isInputGrabbed()) {
        InputSystem::get().setGrabInput(false);
    }

    m_iCurSelSurface = -1;
    m_SceneRenderer.unloadLevel();
    m_vPosition = glm::vec3(0, 0, 0);
    m_vRotation = glm::vec3(0, 0, 0);
}

void MainViewRenderer::tick() {
    ImVec2 newPadding = ImGui::GetStyle().WindowPadding;
    newPadding.x *= 0.33f;
    newPadding.y *= 0.33f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, newPadding);

    if (ImGui::Begin("Main View")) {
        // FOV slider
        float fov = v_fov.getValue();
        ImGui::PushItemWidth(320);
        if (ImGui::SliderFloat("FOV", &fov, 90, 150, "%.f")) {
            v_fov.setValue(fov);
        }
        ImGui::PopItemWidth();

        if (WorldState::get()) {
            bool isGrabbed = InputSystem::get().isInputGrabbed();
            ImVec2 topLeftScreenPos;

            // Viewport
            {
                ImVec2 vMin = imVecFloor(ImGui::GetCursorPos());
                ImVec2 vMax = imVecFloor(ImGui::GetWindowContentRegionMax());
                ImVec2 vSize(vMax.x - vMin.x, vMax.y - vMin.y);
                glm::ivec2 vViewportSize = glm::ivec2(vSize.x, vSize.y);

                if (vViewportSize != m_vViewportSize && vViewportSize.x > 0 &&
                    vViewportSize.y > 0) {
                    m_vViewportSize = vViewportSize;
                    refreshFramebuffer();
                    m_SceneRenderer.setViewportSize(vViewportSize);
                }

                ImVec2 oldCursor = ImGui::GetCursorPos();
                ImGui::Image(m_pBackbufferMat, vSize, ImVec2(0, 1), ImVec2(1, 0));

                ImGui::SetCursorPos(oldCursor);
                topLeftScreenPos = ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton("viewport", vSize);
            }

            // Mouse input
            {
                if (ImGui::IsItemHovered()) {
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                        if (!isGrabbed) {
                            SDL_GetMouseState(&m_SavedMousePos.x, &m_SavedMousePos.y);
                            InputSystem::get().setGrabInput(true);
                        }
                    } else {
                        if (isGrabbed) {
                            InputSystem::get().setGrabInput(false);
                            SDL_WarpMouseInWindow(MainWindowComponent::get().getWindow(),
                                                  m_SavedMousePos.x, m_SavedMousePos.y);
                            isGrabbed = false;
                        }
                    }

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        // Get on-screen cursor pos
                        ImGuiIO &io = ImGui::GetIO();
                        glm::ivec2 mousePos = glm::ivec2(io.MousePos.x - topLeftScreenPos.x,
                                                         io.MousePos.y - topLeftScreenPos.y);

                        EditorMode *editor = BSPViewer::get().getActiveEditor();
                        if (editor) {
                            EditorTool *tool = editor->getActiveTool();
                            if (tool) {
                                tool->onMainViewClicked(mousePos);
                            }
                        }
                    }
                }
            }

            if (isGrabbed) {
                rotateCamera();
                translateCamera();
            }
        } else {
            ImGui::TextUnformatted("Not loaded.");
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();

    if (WorldState::get()) {
        m_SceneRenderer.showDebugDialog("Renderer");
    } else {
        if (ImGui::Begin("Renderer")) {
            ImGui::TextUnformatted("Not loaded.");
        }

        ImGui::End();
    }
}

bool MainViewRenderer::loadingTick() {
    AFW_ASSERT(m_SceneRenderer.isLoading());
    return m_SceneRenderer.loadingTick();
}

void MainViewRenderer::renderMainView() {
    // Scale viewport FOV
    // Assume that v_fov is hor fov at 4:3 aspect ratio
    // Keep vertical fov constant, only scale horizontal
    constexpr float DEFAULT_ASPECT = 4.0f / 3.0f;
    float aspect = (float)m_vViewportSize.x / m_vViewportSize.y;
    float fovx = glm::radians(v_fov.getValue());
    float xtan = tan(fovx / 2);
    float ytan = xtan / DEFAULT_ASPECT;
    xtan = ytan * aspect;
    fovx = glm::degrees(2 * atan(xtan));

    m_flLastFOV = fovx;
    m_uFrameCount++;

    m_SceneRenderer.setPerspective(fovx, aspect, 4, 8192);
    m_SceneRenderer.setPerspViewOrigin(m_vPosition, m_vRotation);
    updateVisibleEnts();
    m_SceneRenderer.renderScene(m_Framebuffer.getId(), 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MainViewRenderer::optimizeBrushModel(Model *model) {
    m_SceneRenderer.optimizeBrushModel(model);
}

Material *MainViewRenderer::getMaterial(const bsp::BSPMipTex &tex) {
    char szName[bsp::MAX_TEXTURE_NAME];
    appfw::strToLower(tex.szName, tex.szName + bsp::MAX_TEXTURE_NAME, szName);

    WADMaterialAssetRef mat = AssetManager::get().findMaterialByName(szName);
    if (mat) {
        return mat->getMaterial();
    } else {
        return nullptr;
    }
}

void MainViewRenderer::drawNormalTriangles(unsigned &drawcallCount) {
    if (m_uBoxCount > 0) {
        m_pBoxMaterial->activateTextures();
        m_pBoxMaterial->enableShader(SHADER_TYPE_CUSTOM_IDX, m_uFrameCount);
        glBindVertexArray(m_BoxVao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, BOX_VERT_COUNT, m_uBoxCount);
        drawcallCount++;
        glBindVertexArray(0);
    }
}

void MainViewRenderer::drawTransTriangles(unsigned &) {}

void MainViewRenderer::updateVisibleEnts() {
    appfw::Prof prof("Entity List");
    m_SceneRenderer.clearEntities();
    auto &ents = WorldState::get()->getEntList();
    unsigned entCount = 0;
    m_uBoxCount = 0;

    for (size_t i = 0; i < ents.size(); i++) {
        BaseEntity *pEnt = ents[i].get();

        if (pEnt->useAABB()) {
            // Draw a box for this entity
            if (m_uBoxCount == MAX_VISIBLE_ENTS) {
                continue;
            }

            glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), pEnt->getAABBHalfExtents() * 2.0f);
            glm::mat4 translate = glm::translate(glm::identity<glm::mat4>(), pEnt->getOrigin() + pEnt->getAABBPos());
            m_BoxInstancesData[m_uBoxCount].color = glm::vec4(pEnt->getAABBColor(), 255) / 255.0f;
            m_BoxInstancesData[m_uBoxCount].transform = translate * scale;
            m_uBoxCount++;
        }

        if (pEnt->getModel() && pEnt->getModel()->getType() == ModelType::Brush) {
            if (entCount == MAX_VISIBLE_ENTS) {
                continue;
            }

            // Check if visible in PVS
            {
                glm::vec3 mins = pEnt->getOrigin() + pEnt->getModel()->getMins();
                glm::vec3 maxs = pEnt->getOrigin() + pEnt->getModel()->getMaxs();
                if (!Vis::get().boxInPvs(m_vPosition, mins, maxs)) {
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
        m_BoxInstances.bind();
        m_BoxInstances.update(0, m_uBoxCount * sizeof(BoxInstance), m_BoxInstancesData.data());
    }
}

void MainViewRenderer::refreshFramebuffer() {
    // Create color buffer
    Texture2D *colorBuffer = static_cast<Texture2D *>(m_pBackbufferMat->getTexture(0));
    colorBuffer->create("Main View");
    colorBuffer->setFilter(TextureFilter::Bilinear);
    colorBuffer->setWrapMode(TextureWrapMode::Clamp);
    colorBuffer->initTexture(GraphicsFormat::RGB8, m_vViewportSize.x, m_vViewportSize.y, false,
                              GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // Attach buffers
    m_Framebuffer.create("Main View");
    m_Framebuffer.attachColor(colorBuffer, 0);

    if (!m_Framebuffer.isComplete()) {
        throw std::logic_error("Main view framebuffer not complete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MainViewRenderer::rotateCamera() {
    int xrel, yrel;
    InputSystem::get().getMouseMovement(xrel, yrel);

    glm::vec3 rot = m_vRotation;

    rot.y -= xrel * m_sens.getValue();
    rot.x += yrel * m_sens.getValue();

    rot.x = std::clamp(rot.x, -MAX_PITCH, MAX_PITCH);
    rot.y = fmod(rot.y, 360.0f);

    m_vRotation = rot;
}

void MainViewRenderer::translateCamera() {
    float delta = cam_speed.getValue() * BSPViewer::get().getTimeDelta();
    glm::vec3 forward, right, up;
    angleVectors(m_vRotation, &forward, &right, &up);

    glm::vec3 direction = glm::vec3(0, 0, 0);
    const uint8_t *keys = SDL_GetKeyboardState(nullptr);

    if (keys[SDL_SCANCODE_W]) {
        direction += forward;
    }

    if (keys[SDL_SCANCODE_S]) {
        direction -= forward;
    }

    if (keys[SDL_SCANCODE_D]) {
        direction += right;
    }

    if (keys[SDL_SCANCODE_A]) {
        direction -= right;
    }

    if (keys[SDL_SCANCODE_LSHIFT]) {
        delta *= 5;
    }

    m_vPosition += direction * delta;
}

Ray MainViewRenderer::screenPointToRay(const glm::vec2 &pos) {
    glm::vec3 forward, right, up;
    angleVectors(m_vRotation, &forward, &right, &up);

    glm::vec2 viewportSize = m_vViewportSize;
    glm::vec2 normPos = pos / viewportSize;
    normPos = 2.0f * normPos - glm::vec2(1.0f, 1.0f); // normPos[i] is [-1; 1]

    normPos.y *= viewportSize.y / viewportSize.x;
    float t = 1.0f / tan(glm::radians(m_flLastFOV) / 2.0f);

    Ray ray;
    ray.origin = m_vPosition;
    ray.direction = glm::normalize(t * forward + normPos.x * right + (-normPos.y) * up);
    return ray;
}
