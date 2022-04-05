#ifndef ASSETS_ASSET_MANAGER_H
#define ASSETS_ASSET_MANAGER_H
#include <string_view>
#include <app_base/app_component.h>
#include <hlviewer/assets/wad_asset.h>
#include <hlviewer/assets/level_asset.h>
#include <hlviewer/assets/sprite_asset.h>

class LevelAsset;
class WADMaterialAsset;

class AssetManager : public AppComponentBase<AssetManager> {
public:
    AssetManager();

    void tick() override;

    //! @returns the list of WADs.
    inline std::list<WADAsset> &getWadList() { return m_Wads; }

    //! Loads a WAD file synchronously. No textures are loaded.
    //! If it was already loaded, returns existing reference.
    WADAsset &loadWad(std::string_view path);

    //! Loads a level synchronously.
    LevelAsset loadLevel(std::string_view path);

    //! Looks for a loaded sprite or returns a null reference.
    SpriteAssetRef findLoadedSprite(std::string_view path);

    //! Loads a sprite or returns the already loaded one.
    SpriteAssetRef loadSprite(std::string_view path);

    //! Sleeps for some time for debugging.
    void fakeDelay();

private:
    std::list<WADAsset> m_Wads;
    std::map<std::string, SpriteAsset> m_Sprites;

    //! @returns path converted to lower case.
    static std::string pathToLower(std::string_view path);
};

#endif
