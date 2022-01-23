#ifndef RENDERER_H
#define RENDERER_H
#include <appfw/utils.h>
#include <graphics/texture2d.h>
#include <graphics/framebuffer.h>
#include <renderer/scene_renderer.h>
#include <renderer/renderer_engine_interface.h>
#include "assets/level_asset.h"

struct Ray;

class MainViewRenderer : public IRendererEngine, appfw::NoMove {
public:
    static inline MainViewRenderer &get() { return *m_spInstance; }

	MainViewRenderer();
    ~MainViewRenderer();

    //! @returns whether world surface are rendered
    bool isWorldRenderingEnabled();

    //! @returns whether entities are rendered
    bool isEntityRenderingEnabled();

    //! @returns direction of ray cast from the view origin into a screen pixel.
    //! @param  pos     Screen position in pixels, origin - top left corner.
    Ray screenPointToRay(const glm::vec2 &pos);

    //! Allows tinting world or entity surfaces.
    //! @param  surface The surface index
    //! @param  color   Color in gamma space
    void setSurfaceTint(int surface, glm::vec4 color);

    //! @returns the material used by the surface.
    inline Material *getSurfaceMaterial(int surface) {
        return m_SceneRenderer.getSurfaceMaterial(surface);
    }

    //! Loads the level, path is passed to scene renderer to load custom lightmaps
    void loadLevel(LevelAssetRef &level);

    //! Unloads the level
    void unloadLevel();

    //! Shows dialogs and processes input
    void tick();

    /**
     * Should be called during loading from main thread.
     * @return  Whether loading has finished or not
     */
    bool loadingTick();

    //! Renders the viewport
    void renderMainView();

    /**
     * Creates an optimized model for a brush model for more effficient solid rendering.
     */
    void optimizeBrushModel(Model *model);

    inline void setLightstyleScale(int lightstyle, float scale) {
        m_SceneRenderer.setLightstyleScale(lightstyle, scale);
    }

    inline glm::vec3 getCameraPos() { return m_vPosition; }
    inline glm::vec3 getCameraRot() { return m_vRotation; }

    inline void setCameraPos(glm::vec3 pos) { m_vPosition = pos; }
    inline void setCameraRot(glm::vec3 rot) { m_vRotation = rot; }

    // IRendererEngine
    Material *getMaterial(const bsp::BSPMipTex &tex) override;
    void drawNormalTriangles(unsigned &drawcallCount) override;
    void drawTransTriangles(unsigned &drawcallCount) override;

private:
    static constexpr int BOX_VERT_COUNT = 6 * 2 * 3; // 6 sides * 2 triangles * 3 verts
    static constexpr float MAX_PITCH = 89.9f;

    struct BoxInstance {
        glm::vec4 color;
        glm::mat4 transform;
    };

    glm::ivec2 m_vViewportSize = glm::ivec2(0, 0);
    SceneRenderer m_SceneRenderer;
    unsigned m_uFrameCount = 0;

    // Entities to render
    std::vector<ClientEntity> m_VisEnts;
    std::vector<BoxInstance> m_BoxInstancesData;
    unsigned m_uBoxCount = 0;

    // Entity boxes
    GLVao m_BoxVao;
    GPUBuffer m_BoxVbo;
    GPUBuffer m_BoxInstances;
    Material *m_pBoxMaterial = nullptr;

    // Backbuffer
    Material *m_pBackbufferMat = nullptr;
    Framebuffer m_Framebuffer;

    // Main view
    glm::vec3 m_vPosition = {0.f, 0.f, 0.f};
    glm::vec3 m_vRotation = {0.f, 0.f, 0.f};
    glm::ivec2 m_SavedMousePos = glm::ivec2(0, 0);
    float m_flLastFOV = 1; // last known FOVx in degrees

    int m_iCurSelSurface = -1;

    void updateVisibleEnts();
    void refreshFramebuffer();
    void rotateCamera();
    void translateCamera();

	static inline MainViewRenderer *m_spInstance = nullptr;
};

#endif
