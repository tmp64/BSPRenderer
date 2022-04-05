#ifndef APP_BASE_BITMAP_H
#define APP_BASE_BITMAP_H
#include <glm/glm.hpp>
#include <vector>

template <typename T>
class Bitmap {
public:
    using PixelType = T;

    inline int getWide() const { return m_iWide; }
    inline int getTall() const { return m_iTall; }
    inline const std::vector<PixelType> &getPixels() { return m_Pixels; }

    //! Allocates the storage.
    void init(int wide, int tall);

    //! Clears the bitmap.
    void clear();

    //! Fills the bitmap with one color.
    void fill(T color);

    //! Copies wide*tall pixels from a buffer to the bitmap.
    //! No bounds checks are performed. Source must be contained by the target completely.
    //! @param  x       Target X coord, accounting for padding
    //! @param  y       Target Y coord, accounting for padding
    //! @param  wide    Width of the bitmet to copy from
    //! @param  tall    Height of the bitmet to copy from
    //! @param  pixels  Array of pixels
    //! @param  padding Number of padding pixels
    void copyPixels(int x, int y, int wide, int tall, const PixelType *pixels, int padding = 0);

private:
    int m_iWide = 0;
    int m_iTall = 0;
    std::vector<PixelType> m_Pixels;
};

template <typename T>
inline void Bitmap<T>::init(int wide, int tall) {
    m_iWide = wide;
    m_iTall = tall;
    m_Pixels.resize(wide * tall);
}

template <typename T>
inline void Bitmap<T>::clear() {
    m_iWide = 0;
    m_iTall = 0;
    m_Pixels.clear();
}

template <typename T>
inline void Bitmap<T>::fill(T color) {
    std::fill(m_Pixels.begin(), m_Pixels.end(), color);
}

template <typename T>
inline void Bitmap<T>::copyPixels(int x, int y, int wide, int tall, const PixelType *pixels,
                                  int padding) {
    int xinner = x + padding;
    int yinner = y + padding;
    for (int i = 0; i < tall; i++) {
        std::copy(pixels + wide * i, pixels + wide * i + wide,
                  m_Pixels.begin() + (yinner + i) * m_iWide + xinner);
    }

    // Fill padding
    if (padding != 0) {
        // Top
        for (int i = y; i < yinner; i++) {
            std::copy(pixels, pixels + wide, m_Pixels.begin() + i * m_iWide + xinner);
        }

        // Bottom
        for (int i = yinner + tall; i < yinner + tall + padding; i++) {
            std::copy(pixels + (tall - 1) * wide, pixels + tall * wide,
                      m_Pixels.begin() + i * m_iWide + xinner);
        }

        // Left
        for (int i = 0; i < tall; i++) {
            std::fill(m_Pixels.begin() + (yinner + i) * m_iWide + x,
                      m_Pixels.begin() + (yinner + i) * m_iWide + x + padding, pixels[wide * i]);
        }

        // Right
        for (int i = 0; i < tall; i++) {
            std::fill(m_Pixels.begin() + (yinner + i) * m_iWide + xinner + wide,
                      m_Pixels.begin() + (yinner + i) * m_iWide + xinner + wide + padding,
                      pixels[wide * (i + 1) - 1]);
        }

        // Top left corner
        T temp = pixels[0];
        for (int i = y; i < yinner; i++) {
            std::fill(m_Pixels.begin() + i * m_iWide + x, m_Pixels.begin() + i * m_iWide + xinner, temp);
        }

        // Top right corner
        temp = pixels[wide - 1];
        for (int i = y; i < yinner; i++) {
            std::fill(m_Pixels.begin() + i * m_iWide + xinner + wide,
                      m_Pixels.begin() + i * m_iWide + xinner + wide + padding, temp);
        }

        // Bottom left corner
        temp = pixels[wide * (tall - 1)];
        for (int i = yinner + tall; i < yinner + tall + padding; i++) {
            std::fill(m_Pixels.begin() + i * m_iWide + x, m_Pixels.begin() + i * m_iWide + xinner, temp);
        }

        // Bottom right corner
        temp = pixels[wide * tall - 1];
        for (int i = yinner + tall; i < yinner + tall + padding; i++) {
            std::fill(m_Pixels.begin() + i * m_iWide + xinner + wide,
                      m_Pixels.begin() + i * m_iWide + xinner + wide + padding, temp);
        }
    }
}

#endif
