#include <fstream>
#include <fmt/format.h>
#include <appfw/appfw.h>
#include <bsp/wad_file.h>

void bsp::WADTexture::getRGB(std::vector<uint8_t> &buffer) const {
    std::lock_guard lock(m_pFile->m_Mutex);
    std::vector<uint8_t> colorTable = readColorTable();
    std::vector<uint8_t> indexTable = readIndexTable();
    buffer.resize((size_t)3 * getWide() * getTall());

    for (unsigned i = 0; i < indexTable.size(); i++) {
        uint8_t index = indexTable[i];
        uint8_t *color = &colorTable[3 * index];
        memcpy(&buffer[3 * i], color, 3);
    }
}

void bsp::WADTexture::getRGBA(std::vector<uint8_t> &buffer) const {
    std::lock_guard lock(m_pFile->m_Mutex);
    std::vector<uint8_t> colorTable = readColorTable();
    std::vector<uint8_t> indexTable = readIndexTable();
    buffer.resize((size_t)4 * getWide() * getTall());

    for (unsigned i = 0; i < indexTable.size(); i++) {
        uint8_t index = indexTable[i];

        if (index != 255) {
            // Solid pixel
            uint8_t *color = &colorTable[3 * index];
            memcpy(&buffer[4 * i], color, 3);
            buffer[4 * i + 3] = 255;
        } else {
            // Transparent pixel
            memset(&buffer[4 * i], 0, 4);
        }
    }
}

void bsp::WADTexture::init(WADFile *pFile, WADDirEntry &dirEntry) {
    m_pFile = pFile;
    
    if (dirEntry.nDiskSize != dirEntry.nSize) {
        throw WADFormatException(fmt::format("file '{}' size mismatch, disk = {}, real = {}",
                                             dirEntry.szName, dirEntry.nDiskSize, dirEntry.nSize));
    }

    m_uFileOffset = dirEntry.nFilePos;
    m_uFileSize = dirEntry.nDiskSize;
    pFile->m_File.seekg(m_uFileOffset);
    pFile->m_File.read(reinterpret_cast<char *>(&m_MipTex), sizeof(m_MipTex));
    m_MipTex.szName[MAX_TEXTURE_NAME - 1] = '\0';
}

std::vector<uint8_t> bsp::WADTexture::readColorTable() const {
    // 2 bytes after the last mipmap entry
    unsigned offset = m_MipTex.nOffsets[MIP_LEVELS - 1] +
                      (getWide() >> (MIP_LEVELS - 1)) * (getTall() >> (MIP_LEVELS - 1)) + 2;

    if (offset + WAD_COLORTABLE_SIZE_BYTES > m_uFileSize) {
        throw WADFormatException("No room for color table in the texture");
    }

    std::vector<uint8_t> tbl(WAD_COLORTABLE_SIZE_BYTES);
    m_pFile->m_File.seekg(m_uFileOffset + offset);
    m_pFile->m_File.read(reinterpret_cast<char *>(tbl.data()), tbl.size());
    return tbl;
}

std::vector<uint8_t> bsp::WADTexture::readIndexTable() const {
    unsigned size = getWide() * getTall();
    unsigned offset = m_MipTex.nOffsets[0];

    if (offset + size > m_uFileSize) {
        throw WADFormatException("No room for the index table");
    }

    std::vector<uint8_t> data(size);
    m_pFile->m_File.seekg(m_uFileOffset + offset);
    m_pFile->m_File.read(reinterpret_cast<char *>(data.data()), data.size());
    return data;
}

bsp::WADFile::WADFile() { 
    m_File.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
}

void bsp::WADFile::loadFromFile(const fs::path &path) {
    try {
        m_File.open(path, std::ifstream::binary);

        // Read WAD header
        WADHeader wadHeader;
        m_File.read(reinterpret_cast<char *>(&wadHeader), sizeof(wadHeader));

        if (memcmp(wadHeader.szMagic, WAD3_MAGIC, sizeof(wadHeader.szMagic)) &&
            memcmp(wadHeader.szMagic, WAD2_MAGIC, sizeof(wadHeader.szMagic))) {
            throw WADFormatException(fmt::format("Magic is invalid. Expected WAD3/WAD2, got {}",
                                                 std::string_view(wadHeader.szMagic, 4)));
        }

        // Read directory
        if (wadHeader.nDir > MAX_WAD_TEXTURES) {
            throw WADFormatException(
                fmt::format("Too many textures ({} > {})", wadHeader.nDir, MAX_WAD_TEXTURES));
        }

        std::vector<WADDirEntry> files(wadHeader.nDir);
        m_File.seekg(wadHeader.nDirOffset);
        m_File.read(reinterpret_cast<char *>(files.data()), sizeof(WADDirEntry) * wadHeader.nDir);

        // Create textures
        m_Textures.resize(files.size());

        for (size_t i = 0; i < files.size(); i++) {
            // Make sure name is null-terminated
            files[i].szName[MAX_TEXTURE_NAME - 1] = '\0';
            m_Textures[i].init(this, files[i]);
        }
    } catch (const std::ios_base::failure &) {
        if (m_File.eof()) {
            throw WADFormatException("unexpected end of file");
        } else {
            throw;
        }
    }
}

void bsp::WADFile::close() {
    m_Textures.clear();
    m_File.close();
}
