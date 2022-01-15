#ifndef GRAPHICS_TEXTURE_CUBE_H
#define GRAPHICS_TEXTURE_CUBE_H
#include <graphics/texture.h>

class TextureCube : public Texture {
public:
    TextureCube();

    inline TextureWrapMode getWrapModeR() const { return m_WrapR; }

    //! Initializes the texture.
    //! Input order is: +X, -X, +Y, -Y, +Z, -Z.
    //! @param  internalFormat      Format in the GPU
    //! @param  wide                Width
    //! @param  tall                Height
    //! @param  generateMipmaps     Create mipmaps for the texture
    //! @param  inputFormat         Format of the input data
    //! @param  inputType           Type of the input data
    //! @param  inputData           Pointer to input image datas. Can be null.
    void initTexture(GraphicsFormat internalFormat, int wide, int tall, bool generateMipmaps,
                     GLenum inputFormat, GLenum inputType, void const *inputData[6]);

    void setWrapMode(TextureWrapMode wrap) override;
    void setWrapModeR(TextureWrapMode wrap);

private:
    TextureWrapMode m_WrapR = TextureDefaults::Wrap;
};

#endif
