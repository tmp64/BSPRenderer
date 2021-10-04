#include "wad_material_asset.h"

WADMaterialAsset::~WADMaterialAsset() {
    if (m_pMaterial) {
        MaterialManager::get().destroyMaterial(m_pMaterial);
        m_pMaterial = nullptr;
    }
}

WADMaterialAsset::UploadTask WADMaterialAsset::init(std::string_view name, int wide, int tall,
                                                    bool isRgba, std::vector<uint8_t> &&data) {
    return UploadTask(
        [this, wide, tall, isRgba, data = std::move(data), name = std::string(name)]() mutable {
            m_pMaterial = MaterialManager::get().createMaterial(name);

            if (isRgba) {
                m_pMaterial->setImageRGBA8(wide, tall, data.data());
            } else {
                m_pMaterial->setImageRGB8(wide, tall, data.data());
            }

            m_bIsLoading = false;
            m_bIsFullyLoaded = true;
        });
}
