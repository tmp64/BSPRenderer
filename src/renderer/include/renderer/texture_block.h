#ifndef RENDERER_TEXTURE_BLOCK_H
#define RENDERER_TEXTURE_BLOCK_H
#include <vector>
#include <glm/glm.hpp>
#include <renderer/raii.h>

namespace detail {

class TextureBlockBase {
public:
    /**
     * Returns width of the block.
     */
    inline int getWide() const { return m_iWide; }

    /**
     * Returns height of the block.
     */
    inline int getTall() const { return m_iTall; }


protected:
    int m_iWide = 0;
    int m_iTall = 0;
    std::vector<uint8_t> m_AllocBits;
    //std::vector<uint16_t> m_FreeCount;

    /**
     * Locates a free block or returns false.
     */
    bool findFreeBlock(int wide, int tall, int &x, int &y);

    /**
     * Marks block as allocated.
     */
    void markBlock(int x, int y, int wide, int tall);

private:
    /**
     * Checks if can fit widextall at (x, y).
     * xout may or may not be set to the pixel before next free x.
     */
    bool isFree(int x, int y, int wide, int tall, int &xout);
};

} // namespace detal

/**
 * Texture block (atlas).
 * Width * Height must be divisable by 8.
 * Maximum width: 65535.
 */
template <typename T>
class TextureBlock : detail::TextureBlockBase {
public:
    TextureBlock() = default;

    /**
     * Creates a texture block with specified size.
     */
    inline TextureBlock(int wide, int tall) { resize(wide, tall); }

    /**
     * Clears the texture block, sets size to 0x0.
     */
    void clear();

    /**
     * Resizes the texture block. The contents will be cleared.
     */
    void resize(int wide, int tall);

    /**
     * Inserts a bitmap into the block.
     * @param   data    Pointer to bitmap bytes, tightly packed with no padding
     * @param   wide    Width of the bitmap
     * @param   tall    Height of the bitmap
     * @param   x       Will be set X coord inside the block
     * @param   y       Will be set Y coord inside the block
     * @param   padding Number of padding pixels
     * @return  True if added successfully, false if not enough space.
     */
    bool insert(const T *data, int wide, int tall, int &x, int &y, int padding = 0);

    /**
     * Returns pointer to pixel buffer.
     */
    inline const T *getData() const { return m_Data.data(); }

private:
    std::vector<T> m_Data;
};

#endif
