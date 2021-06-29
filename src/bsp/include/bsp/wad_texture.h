#ifndef BSP_WAD_TEXTURE_H
#define BSP_WAD_TEXTURE_H
#include <array>
#include <vector>
#include <string>
#include <appfw/utils.h>
#include <appfw/span.h>
#include <bsp/bsp_types.h>

namespace bsp {

class WADTexture {
public:
    WADTexture() = default;
    WADTexture(const BSPMipTex &texture, const appfw::span<uint8_t> file);

    /**
     * Returns name of the texture.
     */
    inline const std::string &getName() const { return m_Name; }

    /**
     * Width of the texture.
     */
    inline int getWide() const { return m_iWide; }

    /**
     * Height of the texture.
     */
    inline int getTall() const { return m_iTall; }

    /**
     * Returns whether or not texture has transparent parts.
     */
    inline bool isTransparent() const { return m_Name[0] == '{'; }

    /**
     * Returns pixels as an RGB array. It's an expensive operation.
     * @param   buffer  A buffer to put the image into
     */
    void getRGBPixels(std::vector<uint8_t> &buffer) const;

    /**
     * Returns pixels as an RGBA array. It's an expensive operation.
     * Only for transparent textures.
     * @param   buffer  A buffer to put the image into
     */
    void getRGBAPixels(std::vector<uint8_t> &buffer) const;

private:
    int m_iWide = 0;
    int m_iTall = 0;
    std::string m_Name;
    appfw::span<uint8_t> m_ColorTable;  // size = 3 * 256 bytes
    appfw::span<uint8_t> m_TexData;     // size = wide * tall, index into the color table
};

} // namespace bsp

#endif
