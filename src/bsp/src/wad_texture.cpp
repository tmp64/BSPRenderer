#include <stdexcept>
#include <bsp/wad_texture.h>

bsp::WADTexture::WADTexture(const BSPMipTex &texture, const appfw::span<uint8_t> file) {
    char szName[MAX_TEXTURE_NAME];
    memcpy(szName, texture.szName, MAX_TEXTURE_NAME);
    szName[MAX_TEXTURE_NAME - 1] = '\0';
    m_Name = szName;
    m_iWide = texture.nWidth;
    m_iTall = texture.nHeight;

    // Only mipmap level 0 is used, the rest are generated in OpenGL using glGenMipMaps
    int dataSize = m_iWide * m_iTall;

    if (texture.nOffsets[0] + dataSize > (int)file.size()) {
        throw std::runtime_error("Texture data offset is invalid");
    }

    m_TexData = file.subspan(texture.nOffsets[0], dataSize);

    // 2 bytes after the last mipmap entry
    int colorTableOffset =
        texture.nOffsets[MIP_LEVELS - 1] + (m_iWide >> (MIP_LEVELS - 1)) * (m_iTall >> (MIP_LEVELS - 1)) + 2;

    if (colorTableOffset + WAD_COLORTABLE_SIZE > file.size()) {
        throw std::runtime_error("No room for color table in the texture");
    }

    m_ColorTable = file.subspan(colorTableOffset, WAD_COLORTABLE_SIZE);
}

void bsp::WADTexture::getRGBPixels(std::vector<uint8_t> &buffer) const {
    buffer.resize((size_t)3 * m_iWide * m_iTall);

    // Pointer to color index of a texture pixel
    const uint8_t *idx = m_TexData.data();

    for (size_t i = 0; i < buffer.size(); i += 3, idx++) {
        const uint8_t *color = m_ColorTable.data() + (*idx * 3);
        buffer[i + 0] = color[0];
        buffer[i + 1] = color[1];
        buffer[i + 2] = color[2];
    }
}

void bsp::WADTexture::getRGBAPixels(std::vector<uint8_t> &buffer) const {
    AFW_ASSERT(isTransparent());

    buffer.resize((size_t)4 * m_iWide * m_iTall);

    // Pointer to color index of a texture pixel
    const uint8_t *idx = m_TexData.data();

    for (size_t i = 0; i < buffer.size(); i += 4, idx++) {
        const uint8_t *color = m_ColorTable.data() + (*idx * 3);

        if (*idx == 255) {
            buffer[i + 0] = 0;
            buffer[i + 1] = 0;
            buffer[i + 2] = 0;
            buffer[i + 3] = 0;
        } else {
            buffer[i + 0] = color[0];
            buffer[i + 1] = color[1];
            buffer[i + 2] = color[2];
            buffer[i + 3] = 255;
        }
    }
}
