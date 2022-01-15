#include <graphics/texture2d.h>

Texture2D::Texture2D() {
    setDimension(GL_TEXTURE_2D, TextureDimension::Tex2D);
}

void Texture2D::initTexture(GraphicsFormat internalFormat, int wide, int tall, bool generateMipmaps,
                            GLenum inputFormat, GLenum inputType, const void *inputData) {
    AFW_ASSERT(!isValid());
    setInternalFormat(internalFormat);
    setSize(wide, tall);
    setHasMipmaps(generateMipmaps);
    
    bind();
    glTexImage2D(getGLTarget(), 0, graphicsFormatToGL(internalFormat), wide, tall, 0, inputFormat,
                 inputType, inputData);

    if (generateMipmaps) {
        glGenerateMipmap(getGLTarget());
    }

    updateMemUsage(calcTextureMemUsage(internalFormat, wide, tall, generateMipmaps));

    // Enable/disable trilinear filtering based on whether there are mipmaps or not.
    setFilter(getFilter());
}

int64_t Texture2D::calcTextureMemUsage(GraphicsFormat internalFormat, int wide, int tall,
                                       bool generateMipmaps) {
    int64_t pixelSize = 0;

    switch (internalFormat) {
    case GraphicsFormat::R8:
        pixelSize = 1;
        break;
    case GraphicsFormat::R16F:
    case GraphicsFormat::RG8:
    case GraphicsFormat::Depth16:
        pixelSize = 2;
        break;
    case GraphicsFormat::RG16F:
    case GraphicsFormat::RGB8:
    case GraphicsFormat::RGBA8:
    case GraphicsFormat::SRGB8:
    case GraphicsFormat::SRGB8_ALPHA8:
    case GraphicsFormat::Depth24:
    case GraphicsFormat::Depth32:
    case GraphicsFormat::Depth24_Stencil8:
        pixelSize = 4;
        break;
    case GraphicsFormat::RGB16F:
        pixelSize = 6;
        break;
    case GraphicsFormat::RGBA16F:
        pixelSize = 8;
        break;
    case GraphicsFormat::None:
    default:
        AFW_ASSERT_REL(false);
    }

    int64_t rowSize = pixelSize * wide;
    int64_t alignment = std::max(pixelSize, (int64_t)4);

    if (rowSize % alignment != 0) {
        rowSize += rowSize % alignment;
    }

    int64_t size = rowSize * tall;

    // With mipmaps size is 4/3 times larger (so add 1/3).
    if (generateMipmaps) {
        size += (GLsizei)(size / 3.0);
    }

    return size;
}
