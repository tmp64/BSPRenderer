#include <bsp/sprite.h>
#include <bsp/entity_key_values.h>
#include <imgui_impl_shaders.h>
#include <hlviewer/assets/asset_manager.h>
#include <hlviewer/assets/asset_loader.h>
#include <hlviewer/scene_view.h>
#include "sprite_viewer.h"

namespace {

const char *s_SpriteTypes[] = {
    "VP_PARALLEL_UPRIGHT", "FACING_UPRIGHT", "VP_PARALLEL", "ORIENTED", "VP_PARALLEL_ORIENTED",
};

const char *s_SpriteFormats[] = {
    "SPR_NORMAL",
    "SPR_ADDITIVE",
    "SPR_INDEXALPHA",
    "SPR_ALPHTEST",
};

const char *s_SyncTypes[] = {
    "ST_SYNC",
    "ST_RAND",
};

const char *s_AvailSkies[] = {"desert", "2desert", "dusk", "night", "alien1", "xen8"};

template <typename E, int N>
void showEnumValue(const char *name, E value, const char *(&values)[N]) {
    int v = (int)value;

    if (v >= 0 && v < N) {
        ImGui::Text("%s: %s", name, values[v]);
    } else {
        ImGui::Text("%s: invalid %d", name, v);
    }
}

class SpriteBlitShader : public ShaderT<SpriteBlitShader> {
public:
    SpriteBlitShader(unsigned type = 0)
        : BaseClass(type) {
        setTitle("SpriteViewer::SpriteBlitShader");
        setVert("assets:shaders/sprite_viewer_2d.vert");
        setFrag("assets:shaders/sprite_viewer_2d.frag");
        setTypes(SHADER_TYPE_BLIT);

        addUniform(m_Sprite, "u_Sprite");
        addUniform(m_Frame, "u_Frame");
        addUniform(m_Color, "u_Color");
    }

    void onShaderCompiled() override { m_Sprite.set(0); }

    TextureVar m_Sprite;
    Var<float> m_Frame;
    Var<glm::vec3> m_Color;
};

SpriteBlitShader s_2DShader;

}

class SpriteViewer::View3D {
public:
    View3D(SpriteViewer &viewer)
        : m_Viewer(viewer) {
        try {
            AssetLoader loader;
            m_Level = loader.loadLevel("assets:maps/dev/sprite_preview.bsp");
            loader.processQueue(true);

            m_pView = std::make_unique<View>(*this, m_Level);
            updateSky(0);
        } catch (const std::exception &e) {
            m_bLoadError = true;
            m_LoadErrorText = e.what();
        }
    }

    void show() {
        if (m_bLoadError) {
            ImGui::TextUnformatted("3D view failed to load:");
            ImGui::TextUnformatted(m_LoadErrorText.c_str());
        } else {
            if (ImGui::Combo("Skybox", &m_iCurSky, s_AvailSkies, (int)std::size(s_AvailSkies))) {
                updateSky(m_iCurSky);
            }

            m_pView->showImage();
        }
    }

    void preRender() {
        if (m_pView) {
            m_pView->setFov(90);
            m_pView->renderBackBuffer();
        }
    }

private:
    class View : public SceneView {
    public:
        View3D &m_View;
        ClientEntity m_Sprite;

        View(View3D &view, LevelAsset &level)
            : SceneView(level)
            , m_View(view) {
            // Set initial camera position
            bsp::EntityKeyValuesDict ents(level.getLevel().getEntitiesLump());
            int ent = ents.findEntityByName("camera_pos");

            if (ent != -1) {
                setCameraPos(ents[ent].get("origin").asFloat3());
                setCameraRot(ents[ent].get("angles").asFloat3());
            } else {
                printw("Sprite preview map is missing camera_pos entity");
            }
        }

        void addVisibleEnts() override {
            switch (m_View.m_Viewer.m_pSprite->getModel().spriteInfo.texFormat) {
            case bsp::SPR_NORMAL: {
                m_Sprite.iRenderMode = kRenderNormal;
                break;
            }
            case bsp::SPR_ADDITIVE: {
                m_Sprite.iRenderMode = kRenderTransAdd;
                break;
            }
            case bsp::SPR_INDEXALPHA:
            case bsp::SPR_ALPHATEST: {
                m_Sprite.iRenderMode = kRenderTransTexture;
                break;
            }
            }

            m_Sprite.vOrigin = {0, 0, 0};
            m_Sprite.vAngles = {0, 0, 0};
            m_Sprite.iRenderFx = kRenderFxNone;
            m_Sprite.iFxAmount = 255;
            m_Sprite.vFxColor = glm::ivec3(m_View.m_Viewer.m_FgColor * 255.0f);
            m_Sprite.pModel = &m_View.m_Viewer.m_pSprite->getModel();
            m_Sprite.flFrame = m_View.m_Viewer.m_flCurrentFrame;
            m_Sprite.flScale = 1.0f;

            if (m_Sprite.vFxColor == glm::ivec3(0, 0, 0)) {
                m_Sprite.vFxColor = glm::ivec3(255, 255, 255);
            }

            addEntity(&m_Sprite);
        }
    };

    SpriteViewer &m_Viewer;
    bool m_bLoadError = false;
    std::string m_LoadErrorText;

    LevelAsset m_Level;
    std::unique_ptr<View> m_pView;
    int m_iCurSky = 0;
    
    void updateSky(int idx) {
        m_iCurSky = idx;
        m_pView->setSkyTexture(s_AvailSkies[idx]);
    }
};

SpriteViewer::SpriteViewer(std::string_view path) {
    // Remove tag from path and set title
    size_t tagEnd = path.find(':');
    setTitle(path.substr(tagEnd + 1));
    setDefaultSize({640, 480});
    m_WindowFlags |= ImGuiWindowFlags_MenuBar;

    // Load the asset
    m_pSprite = AssetManager::get().loadSprite(path);
    const bsp::SpriteInfo &info = m_pSprite->getModel().spriteInfo;

    // Set up 2D view
    std::string texName = "Sprite 2D " + std::string(path);
    auto colorBuffer = std::make_unique<Texture2D>();
    colorBuffer->create(texName);
    colorBuffer->initTexture(GraphicsFormat::RGB8, info.width, info.height, false, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    m_p2DMaterial = MaterialSystem::get().createMaterial(texName);
    m_p2DMaterial->setSize(1, 1);
    m_p2DMaterial->setTexture(0, std::move(colorBuffer));
    m_p2DMaterial->setShader(SHADER_TYPE_IMGUI_IDX, &ImGuiShaderPrototype);
    m_p2DMaterial->setShader(SHADER_TYPE_IMGUI_LINEAR_IDX, &ImGuiShaderPrototype);

    m_2DFramebuffer.create(texName);
    m_2DFramebuffer.attachColor(m_p2DMaterial->getTexture(0), 0);
    if (!m_2DFramebuffer.isComplete()) {
        throw std::logic_error("2D framebuffer is not complete");
    }
}

void SpriteViewer::preRender() {
    DialogBase::preRender();

    if (m_p3DView) {
        m_p3DView->preRender();
    }
}

void SpriteViewer::showContents() {
    const bsp::SpriteInfo &info = m_pSprite->getModel().spriteInfo;

    // Update frame
    if (m_bPlayAnim) {
        m_flCurrentFrame += m_flFramerate * AppBase::getBaseInstance().getTimeDelta();
        m_flCurrentFrame = std::fmodf(m_flCurrentFrame, (float)info.numFrames);
        if (m_flCurrentFrame < 0) {
            m_flCurrentFrame += info.numFrames;
        }
    }

    showMenuBar();
    ImGui::Text("Size: %d x %d", info.width, info.height);

    if (info.numFrames > 1) {
        ImGui::Checkbox("Animate", &m_bPlayAnim);
        ImGui::SameLine();
        ImGui::InputFloat("Framerate", &m_flFramerate);

        int frame = (int)m_flCurrentFrame;
        if (ImGui::SliderInt("##Frame", &frame, 0, info.numFrames - 1)) {
            m_flCurrentFrame = (float)std::clamp(frame, 0, info.numFrames - 1);
        }
        ImGui::SameLine();
        ImGui::Text("%d/%d", frame + 1, info.numFrames);
    }

    ImGui::ColorEdit3("BG", &m_BgColor.r);
    ImGui::ColorEdit3("FG", &m_FgColor.r);

    if (ImGui::BeginTabBar("TabBar")) {
        if (ImGui::BeginTabItem("2D view")) {
            show2D();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("3D view")) {
            show3D();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Info")) {
            showInfo();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void SpriteViewer::showMenuBar() {
    if (ImGui::BeginMenuBar()) {
        // File
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Close")) {
                m_bIsOpen = false;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void SpriteViewer::show2D() {
    const bsp::SpriteInfo &info = m_pSprite->getModel().spriteInfo;
    update2DContents();

    if (ImGui::SliderFloat("Scale", &m_fl2DScale, 0.25f, 8.0f)) {
        m_fl2DScale = std::clamp(m_fl2DScale, 0.25f, 8.0f);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Filter", &m_b2DFilter);

    glm::vec2 size(info.width, info.height);
    size *= m_fl2DScale;
    ImGui::Image(m_p2DMaterial.get(), ImVec2(size.x, size.y));
}

void SpriteViewer::show3D() {
    if (!m_p3DView) {
        m_p3DView = std::make_unique<View3D>(*this);
    }

    m_p3DView->show();
}

void SpriteViewer::showInfo() {
    const bsp::SpriteInfo &info = m_pSprite->getModel().spriteInfo;
    showEnumValue("Type", info.type, s_SpriteTypes);
    showEnumValue("Format", info.texFormat, s_SpriteFormats);
    showEnumValue("Sync type", info.syncType, s_SyncTypes);
    ImGui::Text("Bounding radius: %.2f", info.boundingRadius);
    ImGui::Text("Beam length: %.2f", info.beamLength);
}

void SpriteViewer::update2DContents() {
    const bsp::SpriteInfo &info = m_pSprite->getModel().spriteInfo;

    // Update filtering
    m_p2DMaterial->getTexture(0)->setFilter(m_b2DFilter ? TextureFilter::Bilinear
                                                        : TextureFilter::Nearest);

    // Set up the framebuffer
    glViewport(0, 0, info.width, info.height);
    m_2DFramebuffer.bind(GL_FRAMEBUFFER);
    glClearColor(m_BgColor.r, m_BgColor.g, m_BgColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // Blending
    glEnable(GL_BLEND);
    switch (info.texFormat) {
    case bsp::SPR_ADDITIVE: {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    }
    default: {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }
    }

    // Blit
    ShaderInstance *shaderInstance = s_2DShader.getShaderInstance(SHADER_TYPE_BLIT_IDX);
    shaderInstance->enable(0);
    shaderInstance->getShader<SpriteBlitShader>().m_Frame.set(std::floor(m_flCurrentFrame));
    shaderInstance->getShader<SpriteBlitShader>().m_Color.set(m_FgColor);

    glActiveTexture(GL_TEXTURE0);
    m_pSprite->getModel().spriteMat->getTexture(0)->bind();
    GraphicsStack::get().blit();

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
