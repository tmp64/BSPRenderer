#ifndef HLVIEWER_SCENE_VIEW_H
#define HLVIEWER_SCENE_VIEW_H
#include <appfw/utils.h>
#include <renderer/scene_renderer.h>
#include <renderer/renderer_engine_interface.h>
#include <hlviewer/level_view_renderer_iface.h>
#include <hlviewer/vis.h>
#include <hlviewer/world_state_base.h>
#include <hlviewer/assets/level_asset.h>

class SceneView : public IRendererEngine, public ILevelViewRenderer, appfw::NoMove {
public:
    SceneView(LevelAsset &level);

    //! Sets the WorldStateBase instance to be used by the renderer.
    inline void setWorldState(WorldStateBase *ws) { m_pWorldState = ws; }

    //! Loads and sets the sky texture
    void setSkyTexture(std::string_view skyname);

    //! Shows the ImGui image and handles controls.
    virtual void showImage();

    //! Shows renderer debug dialog.
    void showDebugDialog(const char *title, bool *isVisible = nullptr);

    //! Renders the viewport into a backbuffer.
    //! Must be called before ImGui is displayed.
    void renderBackBuffer();

    //! @returns a ray cast from the view origin into a screen pixel.
    //! @param  pos     Screen position in pixels, origin - top left corner.
    Ray viewportPointToRay(const glm::vec2 &pos);

    //! Allows tinting world or entity surfaces.
    //! @param  surface The surface index
    //! @param  color   Color in gamma space
    void setSurfaceTint(int surface, glm::vec4 color);

    //! @returns the material used by the surface.
    inline Material *getSurfaceMaterial(int surface) {
        return m_SceneRenderer.getSurfaceMaterial(surface);
    }

    inline glm::vec3 getCameraPos() { return m_vPosition; }
    inline glm::vec3 getCameraRot() { return m_vRotation; }

    inline void setCameraPos(glm::vec3 pos) { m_vPosition = pos; }
    inline void setCameraRot(glm::vec3 rot) { m_vRotation = rot; }
    inline void setFov(float fov) { m_flFOV = fov; }
    inline void setShowTriggers(bool state) { m_bShowTriggers = state; }
    inline void setIgnoreWindowFocus(bool state) { m_bIgnoreWindowFocus = state; }

    // IRendererEngine
    Material *getMaterial(const bsp::BSPMipTex &tex) override;
    void drawNormalTriangles(unsigned &drawcallCount) override;
    void drawTransTriangles(unsigned &drawcallCount) override;

    // ILevelViewRenderer
    void setLightstyleScale(int style, float scale) override;

protected:
    //! Handles LMB click.
    virtual void onMouseLeftClick(glm::ivec2 mousePos);

    //! Clears all entity lists.
    virtual void clearEntities();

    //! Adds entities to the scene renderer.
    virtual void addVisibleEnts();

    //! Adds an entity box.
    //! @param  origin      Box center
    //! @param  size        Box size
    //! @param  color       Box color in gamma space.
    //! @param  tintColor   Tinting color.
    void addEntityBox(glm::vec3 origin, glm::vec3 size, glm::vec4 color, glm::vec4 tintColor);

private:
    static constexpr int BOX_VERT_COUNT = 6 * 2 * 3; // 6 sides * 2 triangles * 3 verts
    static constexpr float MAX_PITCH = 89.9f;

    struct BoxInstance {
        glm::vec4 color;
        glm::mat4 transform;
    };

    struct SharedData {
        GPUBuffer boxVbo;
        MaterialPtr boxMaterial = nullptr;

        SharedData();
    };

    // Scene rendering
    LevelAsset &m_Level;
    unsigned m_uFrameCount = 0;
    WorldStateBase *m_pWorldState = nullptr;
    SceneRenderer m_SceneRenderer;
    MaterialPtr m_pSkyboxMaterial = nullptr;

    // Box rendering
    std::shared_ptr<SharedData> m_pSharedData;
    GPUBuffer m_BoxInstances;
    GLVao m_BoxVao;

    // Entities to render
    std::vector<ClientEntity> m_VisEnts;
    std::vector<BoxInstance> m_BoxInstancesData;
    unsigned m_uBoxCount = 0;

    // Backbuffer
    MaterialPtr m_pBackbufferMat = nullptr;
    Framebuffer m_Framebuffer;
    glm::ivec2 m_vViewportSize = glm::ivec2(0, 0);

    // 3D view
    glm::vec3 m_vPosition = {0.f, 0.f, 0.f};
    glm::vec3 m_vRotation = {0.f, 0.f, 0.f};
    glm::ivec2 m_SavedMousePos = glm::ivec2(0, 0);
    float m_flFOV = 1;
    float m_flScaledFOV = 1; // last known FOVx in degrees
    bool m_bShowTriggers = true;
    bool m_bIgnoreWindowFocus = false;

    void createSharedData();
    void createBoxInstanceBuffer();
    void createBoxVao();
    void createSkyboxMaterial();
    void createBackbuffer();

    void showViewport(ImVec2 &topLeftScreenPos);
    void handleMouseInput(ImVec2 topLeftScreenPos);
    void rotateCamera();
    void translateCamera();

    void setViewportSize(glm::ivec2 newSize);

    static inline std::weak_ptr<SharedData> m_spSharedDataWeakPtr;
};

#endif
