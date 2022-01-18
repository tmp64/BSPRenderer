#include <bsp/utils.h>
#include <bsp/entity_key_values.h>
#include "asset_manager.h"
#include "level_asset.h"

//! Maximum number of textures loaded at a time
static constexpr int MAX_TEX_AT_A_TIME = 64;

void LevelAsset::loadFromFile(AssetManager &assMgr, std::string_view path, SharedState &state) {
    m_Level.loadFromFile(getFileSystem().findExistingFile(path));

    state.progress = Progress::LoadingWADs;
    std::vector<WADAssetRef> wads;

    // Find which WADs need to be loaded
    std::vector<std::string> requiredWads = getRequiredWads();

    // Firstly see which WADs are loaded
    std::vector<std::string> loadedWads;
    {
        std::lock_guard lock(assMgr.m_WADMutex);
        loadedWads.reserve(requiredWads.size());

        for (WADAsset &wad : assMgr.m_WADList) {
            for (const std::string &wadName : requiredWads) {
                if (wad.nameEquals(wadName)) {
                    loadedWads.push_back(wadName);
                    wads.push_back(WADAssetRef(&wad));
                    break;
                }
            }
        }
    }

    // Now add WADs that are NOT loaded to the list
    std::vector<std::string> wadsToLoad;

    for (const std::string &reqWad : requiredWads) {
        if (std::find(loadedWads.begin(), loadedWads.end(), reqWad) == loadedWads.end()) {
            wadsToLoad.push_back(reqWad);
        }
    }

    // Load the WADs
    for (const std::string &wadName : wadsToLoad) {
        try {
            wads.push_back(assMgr.loadWADInternal("assets:" + wadName));
        } catch (const std::exception &e) {
            printe("Level {}: failed to load WAD {}: {}", path, wadName, e.what());
        }
    }

    // Save references for loaded textures
    // Remember which textures need to be loaded
    state.progress = Progress::LoadingTextures;
    auto &levelTex = m_Level.getTextures();
    m_Materials.reserve(levelTex.size());

    std::vector<const bsp::BSPMipTex *> texToLoad;
    texToLoad.reserve(levelTex.size());

    for (const bsp::BSPMipTex &tex : levelTex) {
        bool isFound = false;
        for (WADAssetRef &wad : wads) {
            WADMaterialAssetRef mat = wad->findLoadedMaterial(tex.szName);
            if (mat) {
                m_Materials.push_back(std::move(mat));
                isFound = true;
                break;
            }
        }

        if (!isFound) {
            texToLoad.push_back(&tex);
        }
    }

    // Helper function that executes up to MAX_TEX_AT_A_TIME tasks
    // If the limit is reached, waits for all of them to be finished
    std::vector<std::future<void>> texPromises;
    texPromises.reserve(MAX_TEX_AT_A_TIME);

    auto uploadTexture = [&](WADMaterialAsset::UploadTask &&task) {
        if (texPromises.size() == MAX_TEX_AT_A_TIME) {
            // Wait for the last one to finish
            texPromises[MAX_TEX_AT_A_TIME - 1].get();
            texPromises.clear();
        }

        texPromises.push_back(task.get_future());
        AssetManager::get().addMainThreadTask([task = std::move(task)]() mutable { task(); });
    };

    // Load textures
    state.loadedCount = 0;
    state.totalCount = (int)texToLoad.size();

    for (const bsp::BSPMipTex *tex : texToLoad) {
        bool isFound = false;

        for (WADAssetRef &wad : wads) {
            auto [mat, task] = wad->loadMaterialInWorker(tex->szName);

            if (mat) {
                uploadTexture(std::move(task));
                isFound = true;
                break;
            }
        }

        if (!isFound) {
            printe("{}: texture {} not found", path, tex->szName);
        }

        state.loadedCount++;
    }

    // Wait for the last task to finish
    if (!texPromises.empty()) {
        texPromises.rbegin()->get();
    }

    assMgr.fakeDelay();

    m_Path = path;
    m_bIsLoading = false;
    m_bIsFullyLoaded = true;
}

std::vector<std::string> LevelAsset::getRequiredWads() {
    bsp::EntityKeyValuesDict entities(m_Level.getEntitiesLump());
    const bsp::EntityKeyValues &worldspawn = entities[0];
    int wadIdx = worldspawn.indexOf("wad");
    std::string wads = wadIdx != -1 ? worldspawn.get(wadIdx).asString() : "";

    if (wads.empty()) {
        printw("Level doesn't reference any WAD files.");
    }

    return bsp::parseWadListString(wads);
}
