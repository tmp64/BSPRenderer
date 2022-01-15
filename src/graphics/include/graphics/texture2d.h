#ifndef GRAPHICS_TEXTURE_2D_H
#define GRAPHICS_TEXTURE_2D_H
#include <graphics/texture.h>

class Texture2D : public Texture {
public:
    Texture2D();

    //! Initializes the texture.
    //! @param  internalFormat      Format in the GPU
    //! @param  wide                Width
    //! @param  tall                Height
    //! @param  generateMipmaps     Create mipmaps for the texture
    //! @param  inputFormat         Format of the input data
    //! @param  inputType           Type of the input data
    //! @param  inputData           Pointer to input image data. Can be null.
    void initTexture(GraphicsFormat internalFormat, int wide, int tall, bool generateMipmaps,
                     GLenum inputFormat, GLenum inputType, const void *inputData);

    //! @returns an estimate of how much VRAM a texture will take.
    static int64_t calcTextureMemUsage(GraphicsFormat internalFormat, int wide, int tall,
                                       bool generateMipmaps);
};

#endif
