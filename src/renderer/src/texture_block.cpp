#include <renderer/texture_block.h>

TextureBlock::TextureBlock(int wide, int tall) { resize(wide, tall); }

void TextureBlock::clear() {
	m_iWide = 0; 
	m_iTall = 0;
    m_Data.clear();
}

void TextureBlock::resize(int wide, int tall) {
    m_iWide = wide;
    m_iTall = tall;
    m_Data.clear();
    m_Data.resize(BYTES_PER_PX * wide * tall);
}

/*bool TextureBlock::insert(const uint8_t *data, int wide, int tall, int &x, int &y, int padding) {
    //
    return false;
}*/
