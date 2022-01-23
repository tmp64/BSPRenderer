#include <graphics/texture2d_array.h>
#include <graphics/texture2d.h>

Texture2DArray::Texture2DArray() {
    setDimension(GL_TEXTURE_2D_ARRAY, TextureDimension::Tex2DArray);
}

void Texture2DArray::initTexture(GraphicsFormat internalFormat, int wide, int tall, int depth,
                                 bool generateMipmaps, GLenum inputFormat, GLenum inputType,
                                 const void *inputData) {
    AFW_ASSERT(!isValid());
    setInternalFormat(internalFormat);
    setSize(wide, tall);
    m_iDepth = depth;
    setHasMipmaps(generateMipmaps);
    
    bind();
    glTexImage3D(getGLTarget(), 0, graphicsFormatToGL(internalFormat), wide, tall, depth, 0,
                 inputFormat, inputType, inputData);

    if (generateMipmaps) {
        glGenerateMipmap(getGLTarget());
    }

    updateMemUsage(depth *
                   Texture2D::calcTextureMemUsage(internalFormat, wide, tall, generateMipmaps));

    // Enable/disable trilinear filtering based on whether there are mipmaps or not.
    setFilter(getFilter());
}
