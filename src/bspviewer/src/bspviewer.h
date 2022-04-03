#ifndef APP_H
#define APP_H
#include <gui_app_base/gui_app_base.h>
#include <hlviewer/assets/asset_manager.h>
#include <hlviewer/assets/asset_loader.h>
#include "main_view.h"
#include "world_state.h"
#include "editor_mode.h"
#include "surface_editor/surface_edit_mode.h"
#include "entity_editor/entity_edit_mode.h"

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

    //! @returns the map name (without maps/ and .bsp).
    inline const std::string &getMapName() { return m_MapName; }

private:
    AssetManager m_AssetManager;

    // Level loading and display
    std::string m_MapName;
    LevelAsset m_Level;
    std::unique_ptr<AssetLoader> m_pLevelLoader;
    std::unique_ptr<MainView> m_pLevelMainView;
    std::unique_ptr<WorldState> m_pLevelWorldState;
    std::string m_LevelLoadError;
    bool m_bLevelLoadFailed = false;

    // Editor mode support
    std::vector<EditorMode *> m_EditorModes;
    EditorMode *m_pActiveMode = nullptr;

    SurfaceEditMode m_SurfaceEditor;
    EntityEditMode m_EntityEditor;

    void showDockSpace();
    void showMainMenuBar();
    void loadingTick();
    void onLevelLoadingFinished();

    //! Adds the mode to the list.
    void registerMode(EditorMode *mode);

    //! Activates the mode.
    void activateMode(EditorMode *mode);

    //! Calls tick() on the modes that need it.
    void tickModes();

    void showModeSelection();
    void showToolSelection();
    void handleSwitchModesKey();
    void showMainView();
    void showInspector();

    static inline BSPViewer *m_sSingleton = nullptr;
};

inline BSPViewer &BSPViewer::get() { return *m_sSingleton; }

using namespace std::literals::string_literals;

#endif
