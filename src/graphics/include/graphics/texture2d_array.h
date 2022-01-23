#ifndef GRAPHICS_TEXTURE_2D_ARRAY_H
#define GRAPHICS_TEXTURE_2D_ARRAY_H
#include <graphics/texture.h>

class Texture2DArray : public Texture {
public:
    Texture2DArray();

    //! Initializes the texture.
    //! @param  internalFormat      Format in the GPU
    //! @param  wide                Width
    //! @param  tall                Height
    //! @param  depth               Array size
    //! @param  generateMipmaps     Create mipmaps for the texture
    //! @param  inputFormat         Format of the input data
    //! @param  inputType           Type of the input data
    //! @param  inputData           Pointer to input image data. Can be null.
    void initTexture(GraphicsFormat internalFormat, int wide, int tall, int depth,
                     bool generateMipmaps, GLenum inputFormat, GLenum inputType,
                     const void *inputData);

    //! @returns the size of the array.
    inline int getDepth() const { return m_iDepth; }

private:
    int m_iDepth = 0;
};

#endif
