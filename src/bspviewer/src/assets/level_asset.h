#ifndef ASSETS_LEVEL_ASSET_H
#define ASSETS_LEVEL_ASSET_H
#include <bsp/level.h>
#include "asset.h"
#include "wad_asset.h"

class AssetManager;

//! A BSP level.
class LevelAsset : public Asset {
public:
    enum class Progress
    {
        ReadingBSP,
        LoadingWADs,
        LoadingTextures
    };

    struct SharedState {
        std::atomic<Progress> progress = Progress::ReadingBSP;
        std::atomic<int> loadedCount;
        std::atomic<int> totalCount;
    };

    //! Returns a reference to bsp::Level
    inline bsp::Level &getLevel() { return m_Level; }

private:
    bsp::Level m_Level;
    std::vector<WADMaterialAssetRef> m_Materials;

    //! Loads the level from file.
    //! Called by AssetManager from the worker thread.
    void loadFromFile(AssetManager &assMgr, std::string_view path, SharedState &state);

    //! Returns the WAD files needed for the level.
    std::vector<std::string> getRequiredWads();

    friend class AssetManager;
};

using LevelAssetRef = AssetRef<LevelAsset>;
using LevelAssetFuture = AssetFuture<LevelAssetRef, LevelAsset::SharedState>;

#endif
