#include "map_viewer.h"
#include "hlviewer.h"

static ConfigItem<float> map_view_fov("map_view_fov", 110.f, "Map Viewer: Horizontal field of view");

MapViewer::MapViewer(std::string_view path) {
    // Remove tag from path and set title
    size_t tagEnd = path.find(':');
    setTitle(path.substr(tagEnd + 1));
    m_WindowFlags |= ImGuiWindowFlags_MenuBar;

    m_flFov = map_view_fov.getValue();

    try {
        m_pLoader = std::make_unique<AssetLoader>();
        m_Level = m_pLoader->loadLevel(path);
    } catch (const std::exception &e) {
        m_bLoadError = true;
        m_LoadErrorText = e.what();
    }
}

void MapViewer::tick() {
    if (m_pLoader) {
        try {
            m_pLoader->processQueue();

            if (m_pLoader->isFinished()) {
                onLoadingFinished();
            }
        } catch (const std::exception &e) {
            m_bLoadError = true;
            m_LoadErrorText = e.what();
        }
    } else if (m_pWorldState) {
        m_pWorldState->tick(GuiAppBase::getBaseInstance().getTimeDelta());
    }

    ImVec2 newPadding = ImGui::GetStyle().WindowPadding;
    newPadding.x *= 0.33f;
    newPadding.y *= 0.33f;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, newPadding);
    DialogBase::tick();
    ImGui::PopStyleVar();

    if (m_bShowStats) {
        m_pSceneView->showDebugDialog(
            fmt::format("{} - Stats##{}_debug", getTitle(), getUniqueId()).c_str(),
            &m_bShowStats);
    }
}

void MapViewer::preRender() {
    if (isContentVisible() && m_pSceneView) {
        m_pSceneView->setFov(m_flFov);
        m_pSceneView->setShowTriggers(m_bShowTriggers);
        m_pSceneView->renderBackBuffer();
    }
}

void MapViewer::showContents() {
    showMenuBar();

    if (m_bLoadError) {
        showError();
    } else if (m_pLoader) {
        showProgressBar();
    } else {
        showMapView();
    }
}

void MapViewer::onLoadingFinished() {
    appfw::Timer timer;
    m_pLoader = nullptr;
    m_pSceneView = std::make_unique<SceneView>(m_Level);
    m_pWorldState = std::make_unique<WorldStateBase>(m_Level.getLevel(), m_pSceneView.get());
    m_pSceneView->setWorldState(m_pWorldState.get());
    m_pSceneView->setSkyTexture(m_pWorldState->getSkyName());
    printi("MapViewer: Finished in {:.3f} ms", timer.dseconds() * 1000.0);
}

void MapViewer::showMenuBar() {
    if (ImGui::BeginMenuBar()) {
        // File
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Close")) {
                m_bIsOpen = false;
            }
            ImGui::EndMenu();
        }

        // View
        if (ImGui::BeginMenu("View")) {
            // FOV slider
            ImGui::PushItemWidth(320);
            if (ImGui::SliderFloat("FOV", &m_flFov, 90, 150, "%.f")) {
                m_flFov = std::clamp(m_flFov, 1.0f, 189.0f);
                map_view_fov.setValue(m_flFov);
            }
            ImGui::PopItemWidth();

            ImGui::Checkbox("Show triggers", &m_bShowTriggers);
            ImGui::Checkbox("Show statistics", &m_bShowStats);

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void MapViewer::showError() {
    ImGui::TextUnformatted("Map failed to load:");
    ImGui::TextUnformatted(m_LoadErrorText.c_str());
}

void MapViewer::showProgressBar() {
    AFW_ASSERT(m_pLoader);
    int finishedCount = m_pLoader->getFinishedItemCount();
    int totalCount = m_pLoader->getTotalItemCount();
    float percent = totalCount == 0 ? 0.0f : (float)finishedCount / totalCount;
    ImGui::Text("Loading %.f (%d/%d): %s", percent * 100, finishedCount, totalCount,
                m_pLoader->getLastItemName().c_str());
    ImGui::ProgressBar(percent);
}

void MapViewer::showMapView() {
    ImVec2 cursor = ImGui::GetCursorPos();
    m_pSceneView->showImage();

    if (m_bShowStats) {
        ImGui::SetCursorPos(cursor);
        showOverlay();
    }
}

void MapViewer::showOverlay() {
   
    glm::vec3 pos = m_pSceneView->getCameraPos();

    ImGui::Text("FPS: %3.0f", 1.0f / HLViewer::get().getAvgFrameTime());
    ImGui::Text("%.1f %.1f %.1f", pos.x, pos.y, pos.z);
}
