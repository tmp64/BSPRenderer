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

//----------------------------------------------------------------
// Material
//----------------------------------------------------------------
Material::Material(std::nullptr_t) {
    const CheckerboardImage &img = CheckerboardImage::get();
    m_iWide = m_iTall = img.size;

    m_Texture.create();
    glBindTexture(GL_TEXTURE_2D, m_Texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_iWide, m_iTall, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

Material::Material(const bsp::WADTexture &texture, std::vector<uint8_t> &buffer) {
    m_Name = texture.getName();
    m_iWide = texture.getWide();
    m_iTall = texture.getTall();

    m_Texture.create();
    glBindTexture(GL_TEXTURE_2D, m_Texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (texture.isTransparent()) {
        texture.getRGBAPixels(buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.getWide(), texture.getTall(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     buffer.data());
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        texture.getRGBPixels(buffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.getWide(), texture.getTall(), 0, GL_RGB, GL_UNSIGNED_BYTE,
                     buffer.data());
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

//----------------------------------------------------------------
// Material Manager
//----------------------------------------------------------------
MaterialManager &MaterialManager::get() {
    static MaterialManager singleton;
    return singleton;
}

void MaterialManager::init() {
    AFW_ASSERT(m_Materials.empty());
    m_Materials.emplace_back(nullptr);

    if (isAnisoSupported()) {
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &m_flMaxAniso);
    } else {
        printw("Anisotropic filtering not supported by the GPU.");
        printw("Mip-mapping will cause textures to appear blurry.");
        m_flMaxAniso = 0;
    }
}

void MaterialManager::shutdown() {
    m_Map.clear();
    m_Materials.clear();
}

void MaterialManager::tick() {
    appfw::Prof prof("MaterialManager");

    bool bLinear = mat_linear.getValue();
    int iMipMap = std::clamp(mat_mipmap.getValue(), 0, 2);
    int iAniso = std::clamp(mat_aniso.getValue(), 1, (int)m_flMaxAniso);

    if (m_bLinearFiltering != bLinear || m_iMipMapping != iMipMap || m_iAniso != iAniso) {
        int min = 0;
        int mag = 0;

        if (bLinear) {
            // Linear filtering enabled
            mag = GL_LINEAR;

            if (iMipMap == 0) {
                // No mip-mapping
                min = GL_LINEAR;
            } else if (iMipMap == 1) {
                // Nearest mipmap level + linear
                min = GL_LINEAR_MIPMAP_NEAREST;
            } else if (iMipMap == 2) {
                // Linear mipmap level + linear
                min = GL_LINEAR_MIPMAP_LINEAR;
            }
        } else {
            // Linear filtering disabled
            mag = GL_NEAREST;

            if (iMipMap == 0) {
                // No mip-mapping
                min = GL_NEAREST;
            } else if (iMipMap == 1) {
                // Nearest mipmap level + linear
                min = GL_NEAREST_MIPMAP_NEAREST;
            } else if (iMipMap == 2) {
                // Linear mipmap level + linear
                min = GL_NEAREST_MIPMAP_LINEAR;
            }
        }

        // Update for all materials
        for (size_t i = 0; i < m_Materials.size(); i++) {
            m_Materials[i].bindTextures();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
            
            if (isAnisoSupported()) {
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, (float)iAniso);
            }
        }

        m_bLinearFiltering = bLinear;
        m_iMipMapping = iMipMap;
        m_iAniso = iAniso;
    }

    // Show UI
    bool showUI = mat_ui.getValue();
    if (showUI) {
        ImGui::SetNextWindowBgAlpha(0.2f);
        if (ImGui::Begin("Material manager", &showUI)) {
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
        if (showUI != mat_ui.getValue()) {
            mat_ui.setValue(showUI);
        }
    }
}

bool MaterialManager::isWadLoaded(const std::string &name) {
    return std::find(m_LoadedWadNames.begin(), m_LoadedWadNames.end(), name) != m_LoadedWadNames.end();
}

void MaterialManager::loadWadFile(const fs::path &name) {
    std::lock_guard lock(m_Mutex);

    std::string wadFileName = name.filename().u8string();

    if (isWadLoaded(wadFileName)) {
        return;
    }

    try {
        appfw::Timer timer;
        timer.start();

        bsp::WADFile wad(name);
        std::vector<uint8_t> buffer;
        buffer.reserve(512 * 512);  // Reserver up to 512x512 px
        m_Materials.reserve(m_Materials.size() + wad.getTextures().size());

        for (const bsp::WADTexture &tex : wad.getTextures()) {
            auto it = m_Map.find(tex.getName());
            if (it == m_Map.end()) {
                m_Materials.emplace_back(tex, buffer);
                m_Map[tex.getName()] = m_Materials.size() - 1;
            } else {
                printw("{}: texture {} already loaded from other WAD", wadFileName, tex.getName());
            }
        }
        
        timer.stop();
        m_LoadedWadNames.push_back(wadFileName);
        printi("Loaded {} in {:.3f} s", name.filename().u8string(), timer.dseconds());
    }
    catch (const std::exception &e) {
        printe("Failed to load WAD {}: {}", name.filename().u8string(), e.what());
    }
}

size_t MaterialManager::findMaterial(const std::string &name) { 
    std::lock_guard lock(m_Mutex);
    auto it = m_Map.find(name);

    if (it == m_Map.end()) {
        return NULL_MATERIAL;
    } else {
        return it->second;    
    }
}

bool MaterialManager::isAnisoSupported() {
    return GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic;
}
