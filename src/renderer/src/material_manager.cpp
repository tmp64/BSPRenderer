#include <appfw/appfw.h>
#include <appfw/prof.h>
#include <appfw/timer.h>
#include <bsp/wad_file.h>
#include <gui_app_base/imgui_controls.h>
#include <stb_image.h>
#include <renderer/material_manager.h>

ConVar<bool> mat_ui("mat_ui", false, "Enable material manager dev UI");
ConVar<bool> mat_linear("mat_linear", true, "Enable material linear filtering");
ConVar<int> mat_mipmap("mat_mipmap", 2, "Enable material mip-mapping, 2 - also enable mipmap lerping");
ConVar<int> mat_aniso("mat_aniso", 16, "Material anisotropic filtering");

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

Material::Material(std::string_view name) {
    m_Name = name;
    m_Texture.create(name);
}

//----------------------------------------------------------------
// Material
//----------------------------------------------------------------
Material::Material(std::nullptr_t) : Material("null material") {
    const CheckerboardImage &img = CheckerboardImage::get();
    setImageRGB8(img.size, img.size, img.data.data());
}

void Material::setImageRGB8(int wide, int tall, const void *data) {
    initSurface();
    m_iWide = wide;
    m_iTall = tall;
    m_Texture.texImage2D(GL_TEXTURE_2D, 0, GL_RGB8, wide, tall, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Material::setImageRGBA8(int wide, int tall, const void *data) {
    initSurface();
    m_iWide = wide;
    m_iTall = tall;
    m_Texture.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, wide, tall, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Material::initSurface() {
    if (m_Type == Type::None) {
        m_Type = Type::Surface;
        glBindTexture(GL_TEXTURE_2D, m_Texture.getId());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        MaterialManager::get().updateTextureFiltering(GL_TEXTURE_2D);
    }

    AFW_ASSERT_REL_MSG(m_Type == Type::Surface, "Can't change material type after image was set");
}

//----------------------------------------------------------------
// Material Manager
//----------------------------------------------------------------
MaterialManager::MaterialManager() {
    setTickEnabled(true);
    m_Materials.emplace_back(nullptr);

    if (isAnisoSupported()) {
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &m_flMaxAniso);
    } else {
        printw("Anisotropic filtering not supported by the GPU.");
        printw("Mip-mapping will cause textures to appear blurry.");
        m_flMaxAniso = 0;
    }
}

MaterialManager::~MaterialManager() {
    // null material will always be present
    if (m_Materials.size() > 1) {
        printw("MaterialManager: {} material(s) have leaked.", m_Materials.size());
        AFW_ASSERT_MSG(false, "Material resource leak detected");
    }
}

void MaterialManager::tick() {
    appfw::Prof prof("MaterialManager");

    bool bLinear = mat_linear.getValue();
    int iMipMap = std::clamp(mat_mipmap.getValue(), 0, 2);
    int iAniso = std::clamp(mat_aniso.getValue(), 1, (int)m_flMaxAniso);

    if (m_bLinearFiltering != bLinear || m_iMipMapping != iMipMap || m_iAniso != iAniso) {
        m_bLinearFiltering = bLinear;
        m_iMipMapping = iMipMap;
        m_iAniso = iAniso;

        // Update for all materials
        for (Material &mat : m_Materials) {
            if (mat.getType() == Material::Type::Surface) {
                mat.bindSurfaceTextures();
                updateTextureFiltering(GL_TEXTURE_2D);
            }
        }
    }

    // Show UI
    if (mat_ui.getValue()) {
        ImGui::SetNextWindowBgAlpha(0.2f);
        if (ImGuiBeginCvar("Material manager", mat_ui)) {
            ImGui::Text("Count: %d", (int)m_Materials.size());
            CvarCheckbox("Linear filtering", mat_linear);

            const char *mipmapList[] = {"Off", "Nearest", "Linear"};

            if (ImGui::BeginCombo("Mipmap", mipmapList[iMipMap])) {
                for (int i = 0; i < 3; i++) {
                    bool isSelected = i == iMipMap;
                    if (ImGui::Selectable(mipmapList[i], isSelected))
                        mat_mipmap.setValue(i);

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (ImGui::SliderInt("Anisotropic filtering", &iAniso, 1, (int)m_flMaxAniso)) {
                mat_aniso.setValue(iAniso);
            }
        }

        ImGui::End();
    }
}

Material *MaterialManager::getNullMaterial() {
    return &(*m_Materials.begin());
}

Material *MaterialManager::createMaterial(std::string_view name) {
    auto it = m_Materials.emplace(m_Materials.end(), name);
    it->m_Iter = it;
    return &(*it);
}

void MaterialManager::destroyMaterial(Material *mat) {
    m_Materials.erase(mat->m_Iter);
}

bool MaterialManager::isAnisoSupported() {
    return GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic;
}

void MaterialManager::updateTextureFiltering(GLenum target) {
    int min = 0;
    int mag = 0;

    if (m_bLinearFiltering) {
        // Linear filtering enabled
        mag = GL_LINEAR;

        if (m_iMipMapping == 0) {
            // No mip-mapping
            min = GL_LINEAR;
        } else if (m_iMipMapping == 1) {
            // Nearest mipmap level + linear
            min = GL_LINEAR_MIPMAP_NEAREST;
        } else if (m_iMipMapping == 2) {
            // Linear mipmap level + linear
            min = GL_LINEAR_MIPMAP_LINEAR;
        }
    } else {
        // Linear filtering disabled
        mag = GL_NEAREST;

        if (m_iMipMapping == 0) {
            // No mip-mapping
            min = GL_NEAREST;
        } else if (m_iMipMapping == 1) {
            // Nearest mipmap level + linear
            min = GL_NEAREST_MIPMAP_NEAREST;
        } else if (m_iMipMapping == 2) {
            // Linear mipmap level + linear
            min = GL_NEAREST_MIPMAP_LINEAR;
        }
    }

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, min);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mag);

    if (isAnisoSupported()) {
        glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY, (float)m_iAniso);
    }
}
