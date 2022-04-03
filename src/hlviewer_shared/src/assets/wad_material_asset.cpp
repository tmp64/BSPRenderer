#include <appfw/str_utils.h>
#include <graphics/texture2d.h>
#include <imgui_impl_shaders.h>
#include <renderer/scene_renderer.h>
#include <renderer/scene_shaders.h>
#include <hlviewer/assets/wad_material_asset.h>

void WADMaterialAsset::init(std::string name, std::string_view wadName, int wide,
                                   int tall, bool isRgba, const std::vector<uint8_t> &data) {
    appfw::strToLower(name.begin(), name.end());
    m_pMaterial = MaterialSystem::get().createMaterial(name);
    m_pMaterial->setSize(wide, tall);
    m_pMaterial->setWadName(wadName);
    m_pMaterial->setUsesGraphicalSettings(true);
    m_pMaterial->setShader(SHADER_TYPE_WORLD_IDX, &SceneShaders::Shaders::brush);
    m_pMaterial->setShader(SHADER_TYPE_BRUSH_MODEL_IDX, &SceneShaders::Shaders::brush);
    m_pMaterial->setShader(SHADER_TYPE_IMGUI_IDX, &ImGuiShaderPrototype);
    m_pMaterial->setShader(SHADER_TYPE_IMGUI_LINEAR_IDX, &ImGuiShaderPrototype);

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
}
