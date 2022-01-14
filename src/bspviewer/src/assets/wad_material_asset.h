#ifndef ASSETS_MATERIAL_ASSET_H
#define ASSETS_MATERIAL_ASSET_H
#include <material_system/material_system.h>
#include "asset.h"

class AssetManager;

//! A BSP level.
class WADMaterialAsset : public Asset {
public:
    using UploadTask = std::packaged_task<void()>;

    ~WADMaterialAsset();

    //! Returns the material.
    inline Material *getMaterial() const { return m_pMaterial; }

private:
    Material *m_pMaterial = nullptr;

    UploadTask init(std::string_view name, std::string_view wadName, int wide, int tall, bool isRgba,
                    std::vector<uint8_t> &&data);

    friend class WADAsset;
};

using WADMaterialAssetRef = AssetRef<WADMaterialAsset>;
using WADMaterialAssetFuture = AssetFuture<WADMaterialAssetRef>;

#endif
