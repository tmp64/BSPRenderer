#include <fstream>
#include <fmt/format.h>
#include <appfw/services.h>
#include <bsp/wad_file.h>

bsp::WADFile::WADFile(appfw::span<uint8_t> data) { loadFromBytes(data); }

bsp::WADFile::WADFile(const fs::path &filename) { loadFromFile(filename); }

void bsp::WADFile::loadFromFile(const fs::path &path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (file.fail()) {
        throw std::runtime_error(std::string("Failed to open file: ") + strerror(errno));
    }

    file.seekg(0, file.end);
    size_t size = file.tellg();
    file.seekg(0, file.beg);
    std::vector<uint8_t> data;
    data.resize(size);
    file.read((char *)data.data(), size);

    m_WadData = std::move(data);
    loadFromBytesInternal();
}

void bsp::WADFile::loadFromBytes(appfw::span<uint8_t> data) {
    if (data.empty()) {
        throw WADFormatException("File is empty");
    }

    m_WadData.resize(data.size());
    std::copy(data.begin(), data.end(), m_WadData.begin());
}

void bsp::WADFile::loadFromBytesInternal() {
    if (m_WadData.empty()) {
        throw WADFormatException("File is empty");
    }

    // Read WAD header
    WADHeader wadHeader;

    if (m_WadData.size() < sizeof(WADHeader)) {
        throw WADFormatException("File is too small for header");
    }

    memcpy(&wadHeader, m_WadData.data(), sizeof(wadHeader));

    if (memcmp(wadHeader.szMagic, WAD3_MAGIC, sizeof(wadHeader.szMagic)) &&
        memcmp(wadHeader.szMagic, WAD2_MAGIC, sizeof(wadHeader.szMagic))) {
        throw WADFormatException(
            fmt::format("Magic is invalid. Expected WAD3/WAD2, got {}", std::string_view(wadHeader.szMagic, 4)));
    }

    if (m_WadData.size() < wadHeader.nDirOffset ||
        m_WadData.size() - wadHeader.nDirOffset < wadHeader.nDir * sizeof(WADDirEntry)) {
        throw WADFormatException("Invalid dir offset");
    }

    std::vector<WADDirEntry> dirArray;
    dirArray.resize(wadHeader.nDir);
    memcpy(dirArray.data(), m_WadData.data() + wadHeader.nDirOffset, wadHeader.nDir * sizeof(WADDirEntry));

    for (WADDirEntry &dirEntry : dirArray) {
        if (dirEntry.nSize != dirEntry.nDiskSize) {
            throw WADFormatException("Directory has compressed entries");
        }

        if (dirEntry.nFilePos + dirEntry.nDiskSize > (int)m_WadData.size()) {
            throw WADFormatException("Invalid dir entry offset/size");
        }

        if (dirEntry.nDiskSize < sizeof(BSPMipTex)) {
            logWarn("Invalid WAD entry");
            continue;
        }

        BSPMipTex bspTex;
        memcpy(&bspTex, m_WadData.data() + dirEntry.nFilePos, sizeof(BSPMipTex));

        m_Textures.emplace_back(bspTex, appfw::span(m_WadData).subspan(dirEntry.nFilePos, dirEntry.nSize));
    }
}
