#ifndef ASSETS_LEVEL_ASSET_H
#define ASSETS_LEVEL_ASSET_H
#include <bsp/level.h>
#include "asset.h"
#include "wad_asset.h"

class AssetManager;

//! A BSP level.
class LevelAsset : public Asset {
public:
    //! Returns a reference to bsp::Level
    inline bsp::Level &getLevel() { return m_Level; }

    //! @returns list of WADs and textures that need to be loaded.
    std::list<std::pair<WADAsset *, int>> findTexturesToLoad();

    //! Finds a material for the level.
    Material *findMaterial(const char *name);

private:
    bsp::Level m_Level;
    std::vector<WADAsset *> m_Wads;

    //! Loads the level from file.
    //! Called by AssetManager from the worker thread.
    void loadFromFile(AssetManager &assMgr, std::string_view path);

    //! Returns the WAD files needed for the level.
    std::vector<std::string> getRequiredWads();

    friend class AssetManager;
};

#endif
