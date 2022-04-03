#ifndef MAP_VIEWER_H
#define MAP_VIEWER_H
#include <hlviewer/assets/asset_loader.h>
#include <hlviewer/world_state_base.h>
#include <hlviewer/scene_view.h>
#include "dialog_base.h"

class MapViewer : public DialogBase {
public:
    MapViewer(std::string_view path);
    void tick() override;
    void preRender() override;

protected:
    void showContents() override;

private:
    // Configuration
    float m_flFov = 0;
    bool m_bShowTriggers = true;
    bool m_bShowStats = false;

    bool m_bLoadError = false;
    std::string m_LoadErrorText;
    std::unique_ptr<AssetLoader> m_pLoader;
    LevelAsset m_Level;
    std::unique_ptr<SceneView> m_pSceneView;
    std::unique_ptr<WorldStateBase> m_pWorldState;

    void onLoadingFinished();

    void showMenuBar();
    void showError();
    void showProgressBar();
    void showMapView();
    void showOverlay();
};

#endif
