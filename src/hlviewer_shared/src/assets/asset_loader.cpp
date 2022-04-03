#include <appfw/appfw.h>
#include <hlviewer/assets/asset_loader.h>

void AssetLoader::processQueue(bool block) {
    appfw::Timer timer;

    while (!m_Tasks.empty()) {
        auto task = std::move(m_Tasks.front());
        m_Tasks.pop();

        task();

        if (!block && timer.us() >= MAX_TIME_MICROS) {
            break;
        }
    }
}

LevelAsset AssetLoader::loadLevel(std::string_view path) {
    LevelAsset level = AssetManager::get().loadLevel(path);
    m_iTotalItems++;
    m_iFinishedItems++;

    // Add tasks to load the textures
    auto textures = level.findTexturesToLoad();
    for (auto &item : textures) {
        WADAsset *wad = item.first;
        int texIdx = item.second;

        m_Tasks.push([this, wad, texIdx]() {
            auto mat = wad->loadMaterial(texIdx);
            m_LastItemName = mat->getMaterial()->getName();
            m_iFinishedItems++;
        });

        m_iTotalItems++;
    }

    return level;
}
