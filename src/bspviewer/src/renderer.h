#ifndef RENDERER_H
#define RENDERER_H
#include <appfw/utils.h>
#include <renderer/scene_renderer.h>

class Renderer : appfw::NoMove {
public:
    static inline Renderer &get() { return *m_spInstance; }

	Renderer();
    ~Renderer();

    //! Sets size of the viewport.
    void setViewportSize(const glm::ivec2 &size);

    //! Loads the level, path is passed to scene renderer to load custom lightmaps
	void loadLevel(const std::string &path, const char *tag);

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

private:
    SceneRenderer m_SceneRenderer;
    std::vector<ClientEntity> m_VisEnts;

    void addVisibleEnts();

	static inline Renderer *m_spInstance = nullptr;
};

#endif
