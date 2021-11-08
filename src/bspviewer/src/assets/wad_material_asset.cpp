#include <appfw/str_utils.h>
#include "wad_material_asset.h"

WADMaterialAsset::~WADMaterialAsset() {
    if (m_pMaterial) {
        MaterialManager::get().destroyMaterial(m_pMaterial);
        m_pMaterial = nullptr;
    }
}

WADMaterialAsset::UploadTask WADMaterialAsset::init(std::string_view name, std::string_view wadName,
                                                    int wide, int tall, bool isRgba,
                                                    std::vector<uint8_t> &&data) {
    return UploadTask([this, wide, tall, isRgba, data = std::move(data), name = std::string(name),
                       wadName = std::string(wadName)]() mutable {
        appfw::strToLower(name.begin(), name.end());
        m_pMaterial = MaterialManager::get().createMaterial(name);
        m_pMaterial->setWadName(wadName);

        if (isRgba) {
            m_pMaterial->setImageRGBA8(wide, tall, data.data());
        } else {
            m_pMaterial->setImageRGB8(wide, tall, data.data());
        }

        m_bIsLoading = false;
        m_bIsFullyLoaded = true;
    });
}
