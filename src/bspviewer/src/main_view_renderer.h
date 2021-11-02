#ifndef RENDERER_H
#define RENDERER_H
#include <appfw/utils.h>
#include <renderer/gpu_managed_objects.h>
#include <renderer/scene_renderer.h>
#include <renderer/renderer_engine_interface.h>
#include "assets/level_asset.h"

struct Ray;

class MainViewRenderer : public IRendererEngine, appfw::NoMove {
public:
    static inline MainViewRenderer &get() { return *m_spInstance; }

	MainViewRenderer();
    ~MainViewRenderer();

    //! Sets size of the viewport.
    void setViewportSize(const glm::ivec2 &size);

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

    glm::ivec2 m_vViewportSize = glm::ivec2(0, 0);
    SceneRenderer m_SceneRenderer;

    // Entities to render
    std::vector<ClientEntity> m_VisEnts;
    std::vector<glm::mat4> m_BoxTransforms;
    unsigned m_uBoxCount = 0;

    // Entity boxes
    GLVao m_BoxVao;
    GPUBuffer m_BoxVbo;
    GPUBuffer m_BoxInstances;

    // Backbuffer
    GLFramebuffer m_Framebuffer;
    GPUTexture m_ColorBuffer;

    // Main view
    glm::vec3 m_vPosition = {0.f, 0.f, 0.f};
    glm::vec3 m_vRotation = {0.f, 0.f, 0.f};
    glm::ivec2 m_SavedMousePos = glm::ivec2(0, 0);
    float m_flLastFOV = 1; // last known FOVx in degrees

    void updateVisibleEnts();
    void refreshFramebuffer();
    void rotateCamera();
    void translateCamera();

    //! @returns direction of ray cast from the view origin int a screen pixel.
    Ray screenPointToRay(const glm::vec2 &pos);

	static inline MainViewRenderer *m_spInstance = nullptr;
};

#endif
