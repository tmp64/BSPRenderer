#include <graphics/texture_cube.h>
#include <graphics/texture2d.h>

TextureCube::TextureCube() {
    setDimension(GL_TEXTURE_CUBE_MAP, TextureDimension::Cube);
}

void TextureCube::initTexture(GraphicsFormat internalFormat, int wide, int tall,
                              bool generateMipmaps, GLenum inputFormat, GLenum inputType,
                              void const *inputData[6]) {
    AFW_ASSERT(!isValid());
    setInternalFormat(internalFormat);
    setSize(wide, tall);
    setHasMipmaps(generateMipmaps);

    bind();

    for (int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, graphicsFormatToGL(internalFormat),
                     wide, tall, 0, inputFormat, inputType, inputData[i]);
    }
    

    if (generateMipmaps) {
        glGenerateMipmap(getGLTarget());
    }

    updateMemUsage(6 * Texture2D::calcTextureMemUsage(internalFormat, wide, tall, generateMipmaps));

    // Enable/disable trilinear filtering based on whether there are mipmaps or not.
    setFilter(getFilter());
}

void TextureCube::setWrapMode(TextureWrapMode wrap) {
    Texture::setWrapMode(wrap);
    setWrapModeR(wrap);
}

void TextureCube::setWrapModeR(TextureWrapMode wrap) {
    bind();
    glTexParameteri(getGLTarget(), GL_TEXTURE_WRAP_R, wrapToGL(wrap));
    m_WrapR = wrap;
}
