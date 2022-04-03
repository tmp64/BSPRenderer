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
    //! @returns the name of the WAD
    inline const std::string &getWADName() const { return m_WADName; }

    //! @returns the number of textures in the WAD.
    inline int getTextureCount() const { return (int)m_Materials.size(); }

    //! Comapares name with the name of the wad.
    //! @param  name    Name from BSP's worldspawn entity
    bool nameEquals(std::string_view name);

    //! @returns index of a texture or -1.
    int indexOfTexture(const char *name) const;

    //! @returns a loaded texture or nullptr.
    const WADMaterialAsset *getMaterial(int index) const { return m_Materials[index].get(); }

    //! Loads a texture synchronously. If texture not found, returns nyllptr.
    //! @param  index   Index returned by indexOfTexture.
    const WADMaterialAsset *loadMaterial(int index);

    //! Extracts name from virtual path.
    //! @returns "assets:path/to/file.wad" -> "path/to/file.wad"
    static std::string extractNameFromPath(std::string_view path);

private:
    std::string m_WADName;
    bsp::WADFile m_File;
    //std::list<WADMaterialAsset> m_Materials;
    std::vector<std::unique_ptr<WADMaterialAsset>> m_Materials;

    //! Finds a texture in WAD or returns nullptr.
    const bsp::WADTexture *findTexture(const char *name);

    //! Loads the WAD from file.
    void loadFromFile(AssetManager &assMgr, std::string_view path, std::string_view wadName);

    friend class AssetManager;
};

#endif
