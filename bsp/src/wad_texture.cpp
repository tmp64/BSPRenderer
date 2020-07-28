#include <stdexcept>
#include <bsp/wad_texture.h>

bsp::WADTexture::WADTexture(const BSPMipTex &texture, const appfw::span<uint8_t> file) {
    char szName[MAX_TEXTURE_NAME];
    memcpy(szName, texture.szName, MAX_TEXTURE_NAME);
    szName[MAX_TEXTURE_NAME - 1] = '\0';
    m_Name = szName;
    m_iWide = texture.nWidth;
    m_iTall = texture.nHeight;

    for (size_t mipLevel = 0; mipLevel < MIP_LEVELS; mipLevel++) {
        int mipWide = m_iWide >> mipLevel;
        int mipTall = m_iTall >> mipLevel;
        int dataSize = mipWide * mipTall;

        if (texture.nOffsets[mipLevel] + dataSize > (int)file.size()) {
            throw std::runtime_error("Texture data offset is invalid");
        }

        appfw::span<uint8_t> fileData = file.subspan(texture.nOffsets[mipLevel], dataSize);
        std::vector<uint8_t> &data = m_TexData[mipLevel];
        
        data.resize(dataSize);
        memcpy(data.data(), fileData.data(), dataSize);
    }

    int colorTableOffset =
        texture.nOffsets[MIP_LEVELS - 1] + (m_iWide >> (MIP_LEVELS - 1)) * (m_iTall >> (MIP_LEVELS - 1)) + 2;

    if (colorTableOffset + m_ColorTable.size() > file.size()) {
        throw std::runtime_error("No room for color table in the texture");
    }

    memcpy(m_ColorTable.data(), file.data() + colorTableOffset, m_ColorTable.size());
}

std::vector<uint8_t> bsp::WADTexture::getRGBPixels(size_t mipLevel) const { 
    std::vector<uint8_t> data;
    int mipWide = m_iWide >> mipLevel;
    int mipTall = m_iTall >> mipLevel;
    data.resize((size_t)3 * mipWide * mipTall);

    // Pointer to color index of a texture pixel
    const uint8_t *idx = m_TexData[mipLevel].data();

    for (size_t i = 0; i < data.size(); i += 3, idx++) {
        const uint8_t *color = m_ColorTable.data() + (*idx * 3);
        data[i + 0] = color[0];
        data[i + 1] = color[1];
        data[i + 2] = color[2];
    }

    return data;
}

std::vector<uint8_t> bsp::WADTexture::getRGBAPixels(size_t mipLevel) const {
    AFW_ASSERT(isTransparent());

    std::vector<uint8_t> data;
    int mipWide = m_iWide >> mipLevel;
    int mipTall = m_iTall >> mipLevel;
    data.resize((size_t)4 * mipWide * mipTall);

    // Pointer to color index of a texture pixel
    const uint8_t *idx = m_TexData[mipLevel].data();

    for (size_t i = 0; i < data.size(); i += 4, idx++) {
        if (*idx == 255) {
            data[i + 0] = 0;
            data[i + 1] = 0;
            data[i + 2] = 0;
            data[i + 3] = 0;
        } else {
            const uint8_t *color = m_ColorTable.data() + *idx;
            data[i + 0] = color[0];
            data[i + 1] = color[1];
            data[i + 2] = color[2];
            data[i + 3] = 255;
        }
    }

    return data;
}
