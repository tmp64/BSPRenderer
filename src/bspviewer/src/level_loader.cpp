#include <imgui.h>
#include "assets/asset_manager.h"
#include "level_loader.h"
#include "main_view_renderer.h"

LevelLoader::LevelLoader(std::string_view path) {
    m_Status = Status::Level;
    m_Future = AssetManager::get().loadLevel(path);
}

LevelLoader::~LevelLoader() {}

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
            m_Status = Status::Finished;
            return true;
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
