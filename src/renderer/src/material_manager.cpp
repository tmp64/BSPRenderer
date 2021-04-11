#include <appfw/services.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(x) AFW_ASSERT(x)

#include <appfw/timer.h>
#include <bsp/wad_file.h>
#include <renderer/stb_image.h>
#include <renderer/material_manager.h>

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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
}

void MaterialManager::shutdown() {
    m_Map.clear();
    m_Materials.clear();
}

void MaterialManager::addWadFile(const fs::path &name) {
    std::lock_guard lock(m_Mutex);

    try {
        appfw::Timer timer;
        timer.start();

        bsp::WADFile wad(name);
        std::vector<uint8_t> buffer;
        buffer.reserve(512 * 512);  // Reserver up to 512x512 px
        m_Materials.reserve(m_Materials.size() + wad.getTextures().size());

        for (const bsp::WADTexture &tex : wad.getTextures()) {
            m_Materials.emplace_back(tex, buffer);
            m_Map[tex.getName()] = m_Materials.size() - 1;
        }
        
        timer.stop();
        logInfo("Loaded {} in {:.3f} s", name.filename().u8string(), timer.elapsedSeconds());
    }
    catch (const std::exception &e) {
        logError("Failed to load WAD {}: {}", name.filename().u8string(), e.what());
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
