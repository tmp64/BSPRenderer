#include <appfw/appfw.h>
#include <app_base/app_base.h>
#include <gui_app_base/imgui_controls.h>
#include <hlviewer/assets/asset_manager.h>
#include <hlviewer/assets/wad_asset.h>
#include <hlviewer/assets/level_asset.h>

ConVar<float> asset_fake_delay("asset_fake_delay", 0,
                               "Adds an additional delay (seconds) when loading assets");

ConVar<int> asset_max_tick("asset_max_tick", 25, "Maximum ms time to load assets in one tick");
ConVar<bool> asset_debug_ui("asset_debug_ui", false, "Show AssetManager debug info");

AssetManager::AssetManager() {
    asset_debug_ui.setCallback([&](const bool &, const bool &val) {
        setTickEnabled(val);
        return true;
    });

    asset_debug_ui.setValue(asset_debug_ui.getValue());
}

void AssetManager::tick() {
    if (ImGuiBeginCvar("Asset Manager Debug", asset_debug_ui)) {
        if (ImGui::TreeNode("WADs")) {
            // WADs
            for (WADAsset &wad : m_Wads) {
                int texCount = wad.getTextureCount();
                int loadedCount = 0;
                for (int i = 0; i < texCount; i++) {
                    const WADMaterialAsset *mat = wad.getMaterial(i);
                    if (mat) {
                        loadedCount++;
                    }
                }

                if (ImGui::TreeNode(
                        fmt::format("{} ({}/{})", wad.getWADName(), loadedCount, texCount)
                            .c_str())) {
                    // Textures
                    for (int i = 0; i < texCount; i++) {
                        const WADMaterialAsset *mat = wad.getMaterial(i);
                        if (mat) {
                            ImGui::Selectable(mat->getMaterial()->getName().c_str());
                        }
                    }

                    ImGui::Text("Loaded: %d/%d", loadedCount, texCount);
                    ImGui::TreePop();
                }
            }            

            ImGui::TreePop();
        }
    }

    ImGui::End();
}

WADAsset &AssetManager::loadWad(std::string_view path) {
    // See if already loaded
    std::string wadName = WADAsset::extractNameFromPath(path);
    for (WADAsset &wad : m_Wads) {
        if (wad.nameEquals(wadName)) {
            return wad;
        }
    }

    printi("AssetManager: Loading wad {}", path);
    WADAsset &wad = m_Wads.emplace_back();
    wad.loadFromFile(*this, path, wadName);
    return wad;
}

LevelAsset AssetManager::loadLevel(std::string_view path) {
    printi("AssetManager: Loading level {}", path);
    LevelAsset level;
    level.loadFromFile(*this, path);
    return level;
}

void AssetManager::fakeDelay() {
    int delay = (int)(asset_fake_delay.getValue() * 1000);
    if (delay > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
}
