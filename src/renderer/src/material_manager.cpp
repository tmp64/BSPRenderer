#include <appfw/services.h>
#include <bsp/wad_file.h>
#include <renderer/material_manager.h>

//----------------------------------------------------------------
// Material
//----------------------------------------------------------------
Material::Material(std::nullptr_t) {
    constexpr size_t TEX_SIZE = 64;

    m_iWide = m_iTall = TEX_SIZE;

    std::vector<uint8_t> data(TEX_SIZE * TEX_SIZE * 3);
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

    glGenTextures(1, &m_nTexture);
    glBindTexture(GL_TEXTURE_2D, m_nTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_SIZE, TEX_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

Material::Material(const bsp::WADTexture &texture) {
    m_Name = texture.getName();
    m_iWide = texture.getWide();
    m_iTall = texture.getTall();

    glGenTextures(1, &m_nTexture);
    glBindTexture(GL_TEXTURE_2D, m_nTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (texture.isTransparent()) {
        std::vector<uint8_t> data = texture.getRGBAPixels(0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.getWide(), texture.getTall(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     data.data());
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        /*std::array<std::vector<uint8_t>, bsp::MIP_LEVELS> data;
        
        for (size_t i = 0; i < bsp::MIP_LEVELS; i++) {
            data[i] = texture.getRGBPixels(i);
        }*/
        std::vector<uint8_t> data = texture.getRGBPixels(0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.getWide(), texture.getTall(), 0, GL_RGB, GL_UNSIGNED_BYTE,
                     data.data());
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

Material::Material(Material &&from) noexcept {
    if (m_nTexture != 0) {
        glDeleteTextures(1, &m_nTexture);
        m_nTexture = 0;
    }

    m_iWide = from.m_iWide;
    m_iTall = from.m_iTall;

    from.m_iWide = from.m_iTall = 0;

    m_nTexture = from.m_nTexture;
    from.m_nTexture = 0;

    m_Name = from.m_Name;
    from.m_Name.clear();
}

Material::~Material() {
    if (m_nTexture != 0) {
        glDeleteTextures(1, &m_nTexture);
        m_nTexture = 0;
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
}

void MaterialManager::shutdown() {
    m_Map.clear();
    m_Materials.clear();
}

void MaterialManager::addWadFile(const std::string &name) {
    try {
        bsp::WADFile wad(name);

        for (const bsp::WADTexture &tex : wad.getTextures()) {
            m_Materials.emplace_back(tex);
            m_Map[tex.getName()] = m_Materials.size() - 1;
        }
    }
    catch (const std::exception &e) {
        logError("Failed to load WAD {}: {}", name, e.what());
    }
}

size_t MaterialManager::findMaterial(const std::string &name) { 
    auto it = m_Map.find(name);

    if (it == m_Map.end()) {
        return NULL_MATERIAL;
    } else {
        return it->second;    
    }
}
