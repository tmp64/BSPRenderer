#ifndef HLVIEWER_ASSETS_SPRITE_ASSET_H
#define HLVIEWER_ASSETS_SPRITE_ASSET_H
#include <renderer/sprite_model.h>
#include <hlviewer/assets/asset.h>

class SpriteAsset : public Asset {
public:
    //! @returns the sprite model.
    inline SpriteModel &getModel() { return m_Model; }

private:
    SpriteModel m_Model;

    //! Loads a sprite from given path.
    void loadFromFile(AssetManager &assMgr, std::string_view path);

    //! Decodes a frame into RGB values.
    static std::vector<glm::u8vec4> decodeFrame(const std::vector<uint8_t> &palette,
                                                const std::vector<uint8_t> &bitmap,
                                                bsp::SpriteFormat format);

    friend class AssetManager;
};

using SpriteAssetRef = AssetRef<SpriteAsset>;

#endif
