#include <app_base/texture_block.h>

bool detail::TextureBlockBase::findFreeBlock(int wide, int tall, int &xout, int &yout) {
    if (wide > m_iWide || tall > m_iTall) {
        return false;
    }

    for (int y = 0; y < m_iTall - tall; y++) {
        for (int x = 0; x < m_iWide - wide; x++) {
            // Check if can fit wide*tall at (x, y)
            if (isFree(x, y, wide, tall, x)) {
                xout = x;
                yout = y;
                return true;
            }
        }
    }

    return false;
}

void detail::TextureBlockBase::markBlock(int x, int y, int wide, int tall) {
    for (int i = 0; i < tall; i++) {
        for (int j = 0; j < wide; j++) {
            int bitpos = (y + i) * m_iWide + (x + j);
            m_AllocBits[bitpos >> 3] |= (1 << (bitpos & 7));
        }
    }
}

bool detail::TextureBlockBase::isFree(int x, int y, int wide, int tall, int &xout) {
    for (int i = 0; i < tall; i++) {
        for (int j = 0; j < wide; j++) {
            int bitpos = (y + i) * m_iWide + (x + j);
            if (m_AllocBits[bitpos >> 3] & (1 << (bitpos & 7))) {
                if (i == 0) {
                    bitpos -= j;
                    int betterx = j;
                    while (betterx < m_iWide &&
                           m_AllocBits[(bitpos + betterx) >> 3] & (1 << ((bitpos + betterx) & 7))) {
                        betterx++;
                    }
                    xout = x + betterx - 1;
                }
                return false;
            }
        }
    }

    return true;
}

template <typename T>
void TextureBlock<T>::clear() {
    m_iWide = 0;
    m_iTall = 0;
    m_Data.clear();
    m_AllocBits.clear();
}

template <typename T>
void TextureBlock<T>::resize(int wide, int tall) {
    AFW_ASSERT(wide % 8 == 0);
    AFW_ASSERT(tall % 8 == 0);

    m_iWide = wide;
    m_iTall = tall;
    m_Data.clear();
    m_AllocBits.clear();
    m_Data.resize(wide * tall);
    m_AllocBits.resize(wide * tall / 8);
}

template <typename T>
bool TextureBlock<T>::insert(const T *data, int wide, int tall, int &xout, int &yout, int padding) {
    int x, y;

    AFW_ASSERT(wide != 0 && tall != 0);
    
    if (!findFreeBlock(wide + 2 * padding, tall + 2 * padding, x, y)) {
        return false;
    }

    markBlock(x, y, wide + 2 * padding, tall + 2 * padding);
    xout = x + padding;
    yout = y + padding;

    copyTexture(data, wide, tall, xout, yout, padding);

    return true;
}

template <typename T>
void TextureBlock<T>::copyTexture(const T *data, int wide, int tall, int xout, int yout, int padding) {
    int x = xout - padding;
    int y = yout - padding;

    for (int i = 0; i < tall; i++) {
        std::copy(data + wide * i, data + wide * i + wide,
                  m_Data.begin() + (yout + i) * m_iWide + xout);
    }

    // Fill padding
    if (padding != 0) {
        // Top
        for (int i = y; i < yout; i++) {
            std::copy(data, data + wide, m_Data.begin() + i * m_iWide + xout);
        }

        // Bottom
        for (int i = yout + tall; i < yout + tall + padding; i++) {
            std::copy(data + (tall - 1) * wide, data + tall * wide,
                      m_Data.begin() + i * m_iWide + xout);
        }

        // Left
        for (int i = 0; i < tall; i++) {
            std::fill(m_Data.begin() + (yout + i) * m_iWide + x,
                      m_Data.begin() + (yout + i) * m_iWide + x + padding, data[wide * i]);
        }

        // Right
        for (int i = 0; i < tall; i++) {
            std::fill(m_Data.begin() + (yout + i) * m_iWide + xout + wide,
                      m_Data.begin() + (yout + i) * m_iWide + xout + wide + padding,
                      data[wide * (i + 1) - 1]);
        }

        // Top left corner
        T temp = data[0];
        for (int i = y; i < yout; i++) {
            std::fill(m_Data.begin() + i * m_iWide + x, m_Data.begin() + i * m_iWide + xout, temp);
        }

        // Top right corner
        temp = data[wide - 1];
        for (int i = y; i < yout; i++) {
            std::fill(m_Data.begin() + i * m_iWide + xout + wide,
                      m_Data.begin() + i * m_iWide + xout + wide + padding, temp);
        }

        // Bottom left corner
        temp = data[wide * (tall - 1)];
        for (int i = yout + tall; i < yout + tall + padding; i++) {
            std::fill(m_Data.begin() + i * m_iWide + x, m_Data.begin() + i * m_iWide + xout, temp);
        }

        // Bottom right corner
        temp = data[wide * tall - 1];
        for (int i = yout + tall; i < yout + tall + padding; i++) {
            std::fill(m_Data.begin() + i * m_iWide + xout + wide,
                      m_Data.begin() + i * m_iWide + xout + wide + padding, temp);
        }
    }
}

template class TextureBlock<glm::vec3>;
template class TextureBlock<glm::u8vec3>;
