#ifndef APP_H
#define APP_H
#include <gui_app_base/gui_app_base.h>
#include <renderer/scene_renderer.h>
#include "assets/asset_manager.h"
#include "world_state.h"
#include "main_view_renderer.h"
#include "level_loader.h"

class BSPViewer : public GuiAppBase {
public:
    using BaseClass = GuiAppBase;

    static BSPViewer &get();

    BSPViewer();
    ~BSPViewer();

    void tick() override;
    void drawBackground() override;
    void onWindowSizeChange(int wide, int tall) override;

    /**
     * Load a .bsp map
     * @param   name    Name of the map, without maps/ and .bsp.
     */
    void loadLevel(const std::string &name);

    //! Unloads currently loaded level.
    //! Throws if loading a level
    void unloadLevel();

    /**
     * Moves and rotates the camera if input is grabbed.
     */
    void processUserInput();

    inline glm::vec3 getCameraPos() { return m_vPos; }
    inline glm::vec3 getCameraRot() { return m_vRot; }

    inline void setCameraPos(glm::vec3 pos) { m_vPos = pos; }
    inline void setCameraRot(glm::vec3 rot) { m_vRot = rot; }

    inline float getAspectRatio() { return m_flAspectRatio; }

private:
    AssetManager m_AssetManager;
    MainViewRenderer m_Renderer;
    std::unique_ptr<LevelLoader> m_pLevelLoader;
    std::unique_ptr<WorldState> m_pWorldState;

    glm::vec3 m_vPos = {0.f, 0.f, 0.f};
    glm::vec3 m_vRot = {0.f, 0.f, 0.f};
    float m_flAspectRatio = 1.f;

    void loadingTick();

    static inline BSPViewer *m_sSingleton = nullptr;
};

inline BSPViewer &BSPViewer::get() { return *m_sSingleton; }

using namespace std::literals::string_literals;

#endif
