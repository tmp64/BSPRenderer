#include <appfw/str_utils.h>
#include <graphics/texture_cube.h>
#include <imgui_impl_shaders.h>
#include <renderer/envmap.h>
#include <renderer/scene_shaders.h>
#include <renderer/utils.h>
#include <hlviewer/scene_view.h>

static ConVar<float> m_sens("m_sens", 0.15f, "Mouse sensitivity (degrees/pixel)");
static ConVar<float> cam_speed("cam_speed", 1000.f, "Camera speed");

namespace {

//! Shader for displaying backbuffer in ImGui
class MainViewShader : public ImGuiShader {
public:
    MainViewShader(unsigned type = 0)
        : ImGuiShader(type) {
        // Always render in gamma space without sRGB.
        // Otherwise picture becomes much darker due to differentces
        // between gamma and sRGB colorspaces.
        setVert("assets:shaders/imgui/gamma.vert");
        setFrag("assets:shaders/imgui/gamma.frag");
        setFramebufferSRGB(false);
    }

private:
    std::unique_ptr<Shader> createShaderInfoInstance(unsigned typeIdx) override {
        return std::make_unique<MainViewShader>(typeIdx);
    }
};

//! Shader used by entity boxes
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

MainViewShader s_MainViewShader;
EntityBoxShader s_EntityBoxShader;

ImVec2 imVecFloor(ImVec2 vec) {
    return ImVec2(floor(vec.x), floor(vec.y));
}

}

SceneView::SharedData::SharedData() {
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

    boxVbo.create(GL_ARRAY_BUFFER, "Entity Box VBO");
    boxVbo.bind();
    boxVbo.init(sizeof(faceVerts), &faceVerts, GL_STATIC_DRAW);

    boxMaterial = MaterialSystem::get().createMaterial("Entity Box");
    boxMaterial->setSize(1, 1);
    boxMaterial->setShader(SHADER_TYPE_CUSTOM_IDX, &s_EntityBoxShader);

    // Update null material shaders
    Material *nm = MaterialSystem::get().getNullMaterial();
    nm->setShader(SHADER_TYPE_WORLD_IDX, &SceneShaders::Shaders::brush);
    nm->setShader(SHADER_TYPE_BRUSH_MODEL_IDX, &SceneShaders::Shaders::brush);
}

SceneView::SceneView(LevelAsset &level)
    : m_Level(level)
    , m_SceneRenderer(level.getLevel(), level.getPath(), *this) {
    createSharedData();
    createBoxInstanceBuffer();
    createBoxVao();
    createSkyboxMaterial();
    createBackbuffer();

    m_VisEnts.resize(MAX_VISIBLE_ENTS);
    m_BoxInstancesData.resize(MAX_VISIBLE_ENTS);
    m_SceneRenderer.setSkyboxMaterial(m_pSkyboxMaterial.get());
}

void SceneView::setSkyTexture(std::string_view skyname) {
    std::string skyPath = "assets:gfx/env/" + std::string(skyname);
    std::unique_ptr<Texture> cubemap;

    try {
        cubemap = std::make_unique<TextureCube>(loadEnvMapImage("Skybox", skyPath));
    } catch (const std::exception &e) {
        printe("Failed to load sky '{}': {}", skyname, e.what());
        cubemap = std::make_unique<TextureCube>(createErrorEnvMap("Skybox"));
    }

    m_pSkyboxMaterial->setTexture(0, std::move(cubemap));
}

void SceneView::showImage() {
    ImVec2 topLeftScreenPos;
    showViewport(topLeftScreenPos);
    handleMouseInput(topLeftScreenPos);
}

void SceneView::showDebugDialog(const char *title, bool *isVisible) {
    m_SceneRenderer.showDebugDialog(title, isVisible);
}

void SceneView::renderBackBuffer() {
    // Scale viewport FOV
    // Assume that v_fov is hor fov at 4:3 aspect ratio
    // Keep vertical fov constant, only scale horizontal
    constexpr float DEFAULT_ASPECT = 4.0f / 3.0f;
    float aspect = (float)m_vViewportSize.x / m_vViewportSize.y;
    float fovx = glm::radians(m_flFOV);
    float xtan = tan(fovx / 2);
    float ytan = xtan / DEFAULT_ASPECT;
    xtan = ytan * aspect;
    fovx = glm::degrees(2 * atan(xtan));

    m_flScaledFOV = fovx;
    m_uFrameCount++;

    SceneRenderer::ViewContext &viewContext = m_SceneRenderer.getViewContext();
    viewContext.setPerspective(fovx, aspect, 4, 8192);
    viewContext.setPerspViewOrigin(m_vPosition, m_vRotation);
    
    {
        appfw::Prof prof("Entity List");
        clearEntities();
        addVisibleEnts();
    }

    m_SceneRenderer.renderScene(m_Framebuffer.getId(), 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Ray SceneView::viewportPointToRay(const glm::vec2 &pos) {
    glm::vec3 forward, right, up;
    angleVectors(m_vRotation, &forward, &right, &up);

    glm::vec2 viewportSize = m_vViewportSize;
    glm::vec2 normPos = pos / viewportSize;
    normPos = 2.0f * normPos - glm::vec2(1.0f, 1.0f); // normPos[i] is [-1; 1]

    normPos.y *= viewportSize.y / viewportSize.x;
    float t = 1.0f / tan(glm::radians(m_flScaledFOV) / 2.0f);

    Ray ray;
    ray.origin = m_vPosition;
    ray.direction = glm::normalize(t * forward + normPos.x * right + (-normPos.y) * up);
    return ray;
}

void SceneView::setSurfaceTint(int surface, glm::vec4 color) {
    m_SceneRenderer.setSurfaceTint(surface, color);
}

Material *SceneView::getMaterial(const bsp::BSPMipTex &tex) {
    char szName[bsp::MAX_TEXTURE_NAME];
    appfw::strToLower(tex.szName, tex.szName + bsp::MAX_TEXTURE_NAME, szName);
    return m_Level.findMaterial(szName);
}

void SceneView::drawNormalTriangles(unsigned &drawcallCount) {
    if (m_uBoxCount > 0) {
        m_pSharedData->boxMaterial->activateTextures();
        m_pSharedData->boxMaterial->enableShader(SHADER_TYPE_CUSTOM_IDX, m_uFrameCount);
        glBindVertexArray(m_BoxVao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, BOX_VERT_COUNT, m_uBoxCount);
        drawcallCount++;
        glBindVertexArray(0);
    }
}

void SceneView::drawTransTriangles(unsigned &) {}

void SceneView::setLightstyleScale(int style, float scale) {
    m_SceneRenderer.setLightstyleScale(style, scale);
}

void SceneView::onMouseLeftClick(glm::ivec2) {}

void SceneView::clearEntities() {
    m_SceneRenderer.clearEntities();
    m_uBoxCount = 0;
}

void SceneView::addVisibleEnts() {
    if (!m_pWorldState) {
        return;
    }

    auto &ents = m_pWorldState->getEntList();
    unsigned entCount = 0;

    for (size_t i = 0; i < ents.size(); i++) {
        BaseEntity *pEnt = ents[i].get();

        if (pEnt->isTrigger() && !m_bShowTriggers) {
            continue;
        }

        if (pEnt->useAABB()) {
            addEntityBox(pEnt->getOrigin() + pEnt->getAABBPos(), 2.0f * pEnt->getAABBHalfExtents(),
                         glm::vec4(pEnt->getAABBColor(), 255) / 255.0f, pEnt->getAABBTintColor());
        }

        if (pEnt->getModel() && pEnt->getModel()->type == ModelType::Brush) {
            if (entCount == MAX_VISIBLE_ENTS) {
                continue;
            }

            // Check if visible in PVS
            {
                glm::vec3 mins = pEnt->getOrigin() + pEnt->getModel()->vMins;
                glm::vec3 maxs = pEnt->getOrigin() + pEnt->getModel()->vMaxs;
                if (!m_pWorldState->getVis().boxInPvs(m_vPosition, mins, maxs)) {
                    //continue;
                }
            }

            ClientEntity *pClent = &m_VisEnts[entCount];
            pClent->vOrigin = pEnt->getOrigin();
            pClent->vAngles = pEnt->getAngles();
            pClent->iRenderMode = pEnt->getRenderMode();
            pClent->iRenderFx = pEnt->getRenderFx();
            pClent->iFxAmount = pEnt->getFxAmount();
            pClent->vFxColor = pEnt->getFxColor();
            pClent->pModel = pEnt->getModel();

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

void SceneView::addEntityBox(glm::vec3 origin, glm::vec3 size, glm::vec4 color, glm::vec4 tintColor) {
    if (m_uBoxCount == MAX_VISIBLE_ENTS) {
        return;
    }

    glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), size);
    glm::mat4 translate = glm::translate(glm::identity<glm::mat4>(), origin);
    color = glm::mix(color, tintColor, tintColor.a);
    color.a = 1;
    m_BoxInstancesData[m_uBoxCount].color = color;
    m_BoxInstancesData[m_uBoxCount].transform = translate * scale;
    m_uBoxCount++;
}

void SceneView::createSharedData() {
    // Create SharedData if need to
    m_pSharedData = m_spSharedDataWeakPtr.lock();
    if (!m_pSharedData) {
        m_pSharedData = std::make_shared<SharedData>();
        m_spSharedDataWeakPtr = m_pSharedData;
    }
}

void SceneView::createBoxInstanceBuffer() {
    m_BoxInstances.create(GL_ARRAY_BUFFER, "Entity Box Instances");
    m_BoxInstances.bind();
    m_BoxInstances.init(sizeof(BoxInstance) * MAX_VISIBLE_ENTS, nullptr, GL_DYNAMIC_DRAW);
}

void SceneView::createBoxVao() {
    m_BoxVao.create();
    glBindVertexArray(m_BoxVao);

    // Box vertex data
    m_pSharedData->boxVbo.bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3), (void *)0); // position
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3),
                          (void *)sizeof(glm::vec3)); // normal

    // Per-instance data
    m_BoxInstances.bind();

    // Colors
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(BoxInstance),
                          (void *)(0 * sizeof(glm::vec4)));

    glVertexAttribDivisor(2, 1);

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
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

void SceneView::createSkyboxMaterial() {
    m_pSkyboxMaterial = MaterialSystem::get().createMaterial("Skybox");
    m_pSkyboxMaterial->setSize(1, 1);
    m_pSkyboxMaterial->setShader(SHADER_TYPE_WORLD_IDX, &SceneShaders::Shaders::skybox);
}

void SceneView::createBackbuffer() {
    m_pBackbufferMat = MaterialSystem::get().createMaterial("Scene View Backbuffer");
    m_pBackbufferMat->setSize(1, 1);
    m_pBackbufferMat->setShader(SHADER_TYPE_IMGUI_IDX, &s_MainViewShader);
    m_pBackbufferMat->setShader(SHADER_TYPE_IMGUI_LINEAR_IDX, &s_MainViewShader);
    m_pBackbufferMat->setTexture(0, std::make_unique<Texture2D>());
    setViewportSize({1, 1});
}

void SceneView::showViewport(ImVec2 &topLeftScreenPos) {
    // Calc available size
    ImVec2 vMin = imVecFloor(ImGui::GetCursorPos());
    ImVec2 vMax = imVecFloor(ImGui::GetWindowContentRegionMax());
    ImVec2 vSize(vMax.x - vMin.x, vMax.y - vMin.y);
    glm::ivec2 vViewportSize = glm::ivec2(vSize.x, vSize.y);

    // Resize viewport
    setViewportSize(vViewportSize);

    // Show image and invisible button for mouse input
    ImVec2 oldCursor = ImGui::GetCursorPos();
    ImGui::Image(m_pBackbufferMat.get(), vSize, ImVec2(0, 1), ImVec2(1, 0));

    ImGui::SetCursorPos(oldCursor);
    topLeftScreenPos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("viewport", vSize);
}

void SceneView::handleMouseInput(ImVec2 topLeftScreenPos) {
    bool isGrabbed = InputSystem::get().isInputGrabbed();

    if (!m_bIgnoreWindowFocus && !ImGui::IsWindowFocused()) {
        return;
    }

    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            if (!isGrabbed) {
                SDL_GetMouseState(&m_SavedMousePos.x, &m_SavedMousePos.y);
                InputSystem::get().setGrabInput(true);
            }
        } else {
            if (isGrabbed) {
                InputSystem::get().setGrabInput(false);
                SDL_WarpMouseInWindow(MainWindowComponent::get().getWindow(), m_SavedMousePos.x,
                                      m_SavedMousePos.y);
                isGrabbed = false;
            }
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // Get cursor pos in the viewport
            ImGuiIO &io = ImGui::GetIO();
            glm::ivec2 mousePos =
                glm::ivec2(io.MousePos.x - topLeftScreenPos.x, io.MousePos.y - topLeftScreenPos.y);
            onMouseLeftClick(mousePos);
        }
    }

    if (isGrabbed) {
        rotateCamera();
        translateCamera();
    }
}

void SceneView::rotateCamera() {
    int xrel, yrel;
    InputSystem::get().getMouseMovement(xrel, yrel);

    glm::vec3 rot = m_vRotation;

    rot.y -= xrel * m_sens.getValue();
    rot.x += yrel * m_sens.getValue();

    rot.x = std::clamp(rot.x, -MAX_PITCH, MAX_PITCH);
    rot.y = fmod(rot.y, 360.0f);

    m_vRotation = rot;
}

void SceneView::translateCamera() {
    float delta = cam_speed.getValue() * GuiAppBase::getBaseInstance().getTimeDelta();
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

void SceneView::setViewportSize(glm::ivec2 newSize) {
    if (m_vViewportSize == newSize) {
        return;
    }

    m_vViewportSize = newSize;

    // Create color buffer
    Texture2D *colorBuffer = static_cast<Texture2D *>(m_pBackbufferMat->getTexture(0));
    colorBuffer->create("Scene View");
    colorBuffer->setFilter(TextureFilter::Bilinear);
    colorBuffer->setWrapMode(TextureWrapMode::Clamp);
    colorBuffer->initTexture(GraphicsFormat::RGB8, m_vViewportSize.x, m_vViewportSize.y, false,
                             GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // Attach buffers
    m_Framebuffer.create("Scene View");
    m_Framebuffer.attachColor(colorBuffer, 0);

    if (!m_Framebuffer.isComplete()) {
        throw std::logic_error("Scene view framebuffer not complete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_SceneRenderer.setViewportSize(m_vViewportSize);
}
