#ifndef ASSETS_WAD_ASSET_H
#define ASSETS_WAD_ASSET_H
#include <string_view>
#include <shared_mutex>
#include <bsp/wad_file.h>
#include "asset.h"
#include "wad_material_asset.h"

class AssetManager;

//! A WAD file with textures.
class WADAsset : public Asset {
public:
    //! Comapares name with the name of the wad.
    //! @param  name    Name from BSP's worldspawn entity
    bool nameEquals(std::string_view name);

    //! Finds a loaded texture or returns an empty reference.
    WADMaterialAssetRef findLoadedMaterial(const char *name) const;

    //! Loads a texture asynchronously.
    WADMaterialAssetFuture loadMaterial(const char *name);

    //! Loads a texture in this thread. If not found, returns a null reference.
    std::pair<WADMaterialAssetRef, WADMaterialAsset::UploadTask>
    loadMaterialInWorker(const char *name);

private:
    std::string m_WADName;
    mutable std::shared_mutex m_Mutex; // protects the file and the list
    bsp::WADFile m_File;
    std::list<WADMaterialAsset> m_Materials;

    //! Finds a texture in WAD or returns nullptr.
    const bsp::WADTexture *findTexture(const char *name);

    WADMaterialAssetRef findLoadedMaterialInternal(const char *name) const;

    //! Loads the WAD from file.
    //! Called by AssetManager from the worker thread.
    void loadFromFile(AssetManager &assMgr, std::string_view path);

    friend class AssetManager;
};

using WADAssetRef = AssetRef<WADAsset>;

#endif
