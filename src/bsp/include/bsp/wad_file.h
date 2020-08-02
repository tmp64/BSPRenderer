#ifndef BSP_WAD_FILE_H
#define BSP_WAD_FILE_H
#include <stdexcept>
#include <appfw/utils.h>
#include <bsp/bsp_types.h>
#include <bsp/wad_texture.h>

namespace bsp {

class WADFormatException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class WADFile {
public:
    /**
     * Constructs an empty WAD file.
     */
    WADFile() = default;

    /**
     * Loads WAD from supplied data.
     */
    WADFile(appfw::span<uint8_t> data);

    /**
     * Loads WAD from specified .wad file.
     */
    WADFile(const std::string &path);

    /**
     * Loads WAD from a .wad file.
     */
    void loadFromFile(const std::string &path);

    /**
     * Loads WAD from supplied contents of .wad.
     */
    void loadFromBytes(appfw::span<uint8_t> data);

    /**
     * List of textures from the WAD file.
     */
    inline const std::vector<WADTexture> getTextures() const { return m_Textures; }

private:
    std::vector<WADTexture> m_Textures;
};

} // namespace bsp

#endif
