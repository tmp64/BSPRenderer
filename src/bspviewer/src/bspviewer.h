#ifndef APP_H
#define APP_H
#include <gui_app_base/gui_app_base.h>
#include <renderer/scene_renderer.h>
#include "assets/asset_manager.h"
#include "world_state.h"
#include "main_view_renderer.h"
#include "level_loader.h"
#include "editor_mode.h"
#include "surface_editor/surface_edit_mode.h"

class BSPViewer : public GuiAppBase {
public:
    using BaseClass = GuiAppBase;

    static BSPViewer &get();

    BSPViewer();
    ~BSPViewer();

    void beginTick() override;
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

    //! @returns the currently active edit mode or nullptr.
    inline EditorMode *getActiveEditor() { return m_pActiveMode; }

private:
    AssetManager m_AssetManager;
    MainViewRenderer m_Renderer;
    std::unique_ptr<LevelLoader> m_pLevelLoader;
    std::unique_ptr<WorldState> m_pWorldState;

    // Editor mode support
    std::vector<EditorMode *> m_EditorModes;
    EditorMode *m_pActiveMode = nullptr;

    SurfaceEditMode m_SurfaceEditor;

    void showDockSpace();
    void showMainMenuBar();
    void loadingTick();

    //! Adds the mode to the list.
    void registerMode(EditorMode *mode);

    //! Activates the mode.
    void activateMode(EditorMode *mode);

    //! Calls tick() on the modes that need it.
    void tickModes();

    void showModeSelection();
    void showToolSelection();

    static inline BSPViewer *m_sSingleton = nullptr;
};

inline BSPViewer &BSPViewer::get() { return *m_sSingleton; }

using namespace std::literals::string_literals;

#endif
