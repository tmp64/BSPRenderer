#include <appfw/str_utils.h>
#include <graphics/texture2d.h>
#include <renderer/scene_renderer.h>
#include "wad_material_asset.h"

WADMaterialAsset::~WADMaterialAsset() {
    if (m_pMaterial) {
        MaterialSystem::get().destroyMaterial(m_pMaterial);
        m_pMaterial = nullptr;
    }
}

WADMaterialAsset::UploadTask WADMaterialAsset::init(std::string_view name, std::string_view wadName,
                                                    int wide, int tall, bool isRgba,
                                                    std::vector<uint8_t> &&data) {
    return UploadTask([this, wide, tall, isRgba, data = std::move(data), name = std::string(name),
                       wadName = std::string(wadName)]() mutable {
        appfw::strToLower(name.begin(), name.end());
        m_pMaterial = MaterialSystem::get().createMaterial(name);
        m_pMaterial->setSize(wide, tall);
        m_pMaterial->setWadName(wadName);
        m_pMaterial->setUsesGraphicalSettings(true);
        m_pMaterial->setShader(SceneRenderer::getDefaultSurfaceShader());

        auto texture = std::make_unique<Texture2D>();
        texture->create(name);
        texture->setWrapMode(TextureWrapMode::Repeat);

        if (isRgba) {
            texture->initTexture(GraphicsFormat::RGBA8, wide, tall, true, GL_RGBA, GL_UNSIGNED_BYTE,
                                 data.data());
        } else {
            texture->initTexture(GraphicsFormat::RGB8, wide, tall, true, GL_RGB, GL_UNSIGNED_BYTE,
                                 data.data());
        }

        m_pMaterial->setTexture(0, std::move(texture));

        m_bIsLoading = false;
        m_bIsFullyLoaded = true;
    });
}
