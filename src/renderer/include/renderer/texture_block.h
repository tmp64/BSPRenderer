#ifndef RENDERER_TEXTURE_BLOCK_H
#define RENDERER_TEXTURE_BLOCK_H
#include <vector>
#include <glm/glm.hpp>
#include <renderer/raii.h>

class TextureBlock {
public:
    TextureBlock() = default;

    /**
     * Creates a texture block with specified size.
     */
    TextureBlock(int wide, int tall);

    /**
     * Clears the texture block (sets size to 0x0).
     */
    void clear();

    /**
     * Resizes the texture block. The contents will be cleared.
     */
    void resize(int wide, int tall);

    /**
     * Inserts a bitmap into the block.
     * @param   data    Pointer to bitmap bytes
     * @param   wide    Width of the bitmap
     * @param   tall    Height of the bitmap
     * @param   x       Will be set X coord inside the block
     * @param   y       Will be set Y coord inside the block
     * @param   padding Number of padding pixels
     * @return  True if added successfully, false if not enough space.
     */
    bool insert(const uint8_t *data, int wide, int tall, int &x, int &y, int padding = 0);

private:
    static constexpr int BYTES_PER_PX = 3;

    int m_iWide = 0;
    int m_iTall = 0;
    std::vector<uint8_t> m_Data;
};

#endif
