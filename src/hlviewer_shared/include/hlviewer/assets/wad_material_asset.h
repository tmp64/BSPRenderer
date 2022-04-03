#ifndef ASSETS_MATERIAL_ASSET_H
#define ASSETS_MATERIAL_ASSET_H
#include <material_system/material_system.h>
#include "asset.h"

class AssetManager;

//! A BSP level.
class WADMaterialAsset : public Asset {
public:
    //! Returns the material.
    inline Material *getMaterial() const { return m_pMaterial.get(); }

private:
    MaterialPtr m_pMaterial;

    void init(std::string name, std::string_view wadName, int wide, int tall,
                     bool isRgba, const std::vector<uint8_t> &data);

    friend class WADAsset;
};

#endif
