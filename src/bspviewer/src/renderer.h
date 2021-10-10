#ifndef RENDERER_H
#define RENDERER_H
#include <appfw/utils.h>
#include <renderer/gpu_managed_objects.h>
#include <renderer/scene_renderer.h>
#include <renderer/renderer_engine_interface.h>
#include "assets/level_asset.h"

class Renderer : public IRendererEngine, appfw::NoMove {
public:
    static inline Renderer &get() { return *m_spInstance; }

	Renderer();
    ~Renderer();

    //! Sets size of the viewport.
    void setViewportSize(const glm::ivec2 &size);

    //! Loads the level, path is passed to scene renderer to load custom lightmaps
    void loadLevel(LevelAssetRef &level);

    //! Unloads the level
    void unloadLevel();

    //! Does main loop thinking (e.g. shows debug dialog)
    void tick();

    /**
     * Should be called during loading from main thread.
     * @return  Whether loading has finished or not
     */
    bool loadingTick();

    //! Renders the image into main framebuffer
    void draw();

    /**
     * Creates an optimized model for a brush model for more effficient solid rendering.
     */
    void optimizeBrushModel(Model *model);

    // IRendererEngine
    Material *getMaterial(const bsp::BSPMipTex &tex) override;
    void drawNormalTriangles(unsigned &drawcallCount) override;
    void drawTransTriangles(unsigned &drawcallCount) override;

private:
    static constexpr int BOX_VERT_COUNT = 6 * 2 * 3; // 6 sides * 2 triangles * 3 verts

    SceneRenderer m_SceneRenderer;
    std::vector<ClientEntity> m_VisEnts;
    std::vector<glm::mat4> m_BoxTransforms;
    unsigned m_uBoxCount = 0;

    GLVao m_BoxVao;
    GPUBuffer m_BoxVbo;
    GPUBuffer m_BoxInstances;

    void updateVisibleEnts();

	static inline Renderer *m_spInstance = nullptr;
};

#endif
