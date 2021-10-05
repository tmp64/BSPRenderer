#include <imgui.h>
#include "assets/asset_manager.h"
#include "level_loader.h"
#include "renderer.h"

LevelLoader::LevelLoader(std::string_view path) {
    m_Status = Status::Level;
    m_Future = AssetManager::get().loadLevel(path);
}

LevelLoader::~LevelLoader() {
    if (m_Status == Status::Renderer) {
        // Wait for renderer to finish loading
        while (!Renderer::get().loadingTick()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    } else {
        // Asset manager will clean up itself
    }
}

bool LevelLoader::tick() {
    const char *action = "< text not set >";
    int loadedCount = -1;
    int totalCount = -1;

    switch (m_Status) {
    case Status::Level: {
        if (m_Future.isReady()) {
            // BSP is loaded
            m_pLevel = m_Future.get();
            m_Future = LevelAssetFuture();

            printi("Setting up the renderer...");
            m_Status = Status::Renderer;
            Renderer::get().loadLevel(m_pLevel);
        } else {
            // Update progress status
            std::shared_ptr<LevelAsset::SharedState> state = m_Future.getState();
            using LP = LevelAsset::Progress;

            LP progress = state->progress;
            totalCount = state->totalCount;
            loadedCount = state->totalCount;

            if (progress == LP::ReadingBSP) {
                action = "Loading the BSP";
            } else if (progress == LP::LoadingWADs) {
                action = "Loading WADs";
            } else if (progress == LP::LoadingTextures) {
                action = "Loading textures";
            } else {
                AFW_ASSERT(false);
            }
        }
        break;
    }
    case Status::Renderer: {
        action = "Setting up the renderer";
        
        if (Renderer::get().loadingTick()) {
            // Finished
            m_Status = Status::Finished;
            return true;
        }

        break;
    }
    }

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
    ImVec2 pos = ImGui::GetMainViewport()->Size;
    pos.x /= 2.0f;
    pos.y /= 2.0f;
    ImGui::SetNextWindowPos(pos, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480, 160));

    if (ImGui::Begin("Loading the level", nullptr, flags)) {
        bool showProgress = loadedCount != -1 && totalCount != -1;

        if (showProgress) {
            ImGui::Text("%s (%d/%d)", action, loadedCount, totalCount);
            ImGui::ProgressBar((float)loadedCount / totalCount);
        } else {
            ImGui::Text("%s", action);
        }
    }

    ImGui::End();
    return false;
}
