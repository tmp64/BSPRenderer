#ifndef BSP_WAD_FILE_H
#define BSP_WAD_FILE_H
#include <stdexcept>
#include <mutex>
#include <appfw/utils.h>
#include <appfw/span.h>
#include <bsp/bsp_types.h>

namespace bsp {

class WADFile;

class WADFormatException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class WADTexture {
public:
    //! Returns the name of the texture.
    const char *getName() const { return m_MipTex.szName; }

    //! Returns the width of the texture
    int getWide() const { return m_MipTex.nWidth; }

    //! Returns the height of the texture
    int getTall() const { return m_MipTex.nHeight; }

    //! Returns whether or not texture has transparent parts.
    inline bool isTransparent() const { return m_MipTex.szName[0] == '{'; }

    //! Returns pixels as an RGB array.
    //! @param   buffer  A buffer to put the image into
    void getRGB(std::vector<uint8_t> &buffer) const;

    //! Returns pixels as an RGBA array. Only for transparent textures.
    //! @param   buffer  A buffer to put the image into
    void getRGBA(std::vector<uint8_t> &buffer) const;

private:
    WADFile *m_pFile = nullptr;
    unsigned m_uFileOffset = 0;
    unsigned m_uFileSize = 0;
    BSPMipTex m_MipTex;

    void init(WADFile *pFile, WADDirEntry &dirEntry);
    std::vector<uint8_t> readColorTable() const;
    std::vector<uint8_t> readIndexTable() const;

    friend class WADFile;
};

class WADFile {
public:
    //! Maximum number of textures in a WAD file (some sane value, real maximum is INT32_MAX)
    static constexpr unsigned MAX_WAD_TEXTURES = 16384;

    WADFile();

    //! Loads the list of textures from a .wad file.
    //! The file will be kept open until it is closed.
    //! No textures are loaded into memory until explicitly requested.
    void loadFromFile(const fs::path &path);

    //! Closes the file.
    void close();

    //! List of textures from the WAD file.
    inline const std::vector<WADTexture> &getTextures() const { return m_Textures; }

private:
    std::mutex m_Mutex;
    std::ifstream m_File;
    std::vector<WADTexture> m_Textures;

    friend class WADTexture;
};

} // namespace bsp

#endif
