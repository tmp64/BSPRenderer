#include <material_system/material.h>
#include <material_system/material_system.h>
#include <material_system/shader_instance.h>
#include <material_system/shader.h>

//----------------------------------------------------------------
// CheckerboardImage
//----------------------------------------------------------------
CheckerboardImage::CheckerboardImage() {
    constexpr size_t TEX_SIZE = 64;
    size = TEX_SIZE;

    data.resize(TEX_SIZE * TEX_SIZE * 3);
    uint8_t color[2][3] = {{0, 0, 0}, {255, 0, 255}};

    for (size_t i = 0; i < TEX_SIZE * 3; i += 6) {
        data[i + 0] = color[0][0];
        data[i + 1] = color[0][1];
        data[i + 2] = color[0][2];
        data[i + 3] = color[1][0];
        data[i + 4] = color[1][1];
        data[i + 5] = color[1][2];
    }

    for (size_t i = TEX_SIZE * 3; i < 2 * TEX_SIZE * 3; i += 6) {
        data[i + 0] = color[1][0];
        data[i + 1] = color[1][1];
        data[i + 2] = color[1][2];
        data[i + 3] = color[0][0];
        data[i + 4] = color[0][1];
        data[i + 5] = color[0][2];
    }

    for (size_t i = 2; i < TEX_SIZE; i += 2) {
        memcpy(data.data() + TEX_SIZE * 3 * i, data.data(), TEX_SIZE * 3 * 2);
    }
}

const CheckerboardImage &CheckerboardImage::get() {
    static CheckerboardImage img;
    return img;
}

//----------------------------------------------------------------
// Material
//----------------------------------------------------------------
Material::Material(std::string_view name) {
    m_Name = name;
}

void Material::activateTextures() const {
    AFW_ASSERT(m_iWide > 0 && m_iTall > 0);

    for (int i = 0; i < MAX_TEXTURES; i++) {
        if (m_pOwnTextures[i]) {
            AFW_ASSERT(m_pOwnTextures[i]->isValid());
            glActiveTexture(GL_TEXTURE0 + i);
            m_pOwnTextures[i]->bind();
        }
    }
}

void Material::enableShader(unsigned typeIdx) const {
    ShaderInstance *shaderInstance = nullptr;

    if (m_pShader) {
        shaderInstance = m_pShader->getShaderInstance(typeIdx);
    }

    if (!shaderInstance) {
        shaderInstance = MaterialSystem::get().getFallbackShader()->getShaderInstance(typeIdx);
    }
    
    AFW_ASSERT_MSG(shaderInstance, "Fallback shader is invalid");
    shaderInstance->enable();
}

void Material::setSize(int wide, int tall) {
    AFW_ASSERT(wide > 0 && tall > 0);
    m_iWide = wide;
    m_iTall = tall;
}

void Material::setWadName(std::string_view wadName) {
    m_WadName = wadName;
}

void Material::setTexture(int idx, std::unique_ptr<Texture> &&pTexture) {
    AFW_ASSERT(idx >= 0 && idx < MAX_TEXTURES);
    m_pOwnTextures[idx] = std::move(pTexture);
}

void Material::setUsesGraphicalSettings(bool value) {
    m_bUseGraphicalSettings = value;
}

void Material::setShader(Shader *shader) {
    m_pShader = shader;
}
