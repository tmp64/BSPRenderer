#include <graphics/texture.h>
#include <graphics/graphics_stack.h>

Texture::~Texture() {
    destroy();
}

void Texture::create(std::string_view name) {
    destroy();
    m_Texture.create();
    m_Name = name;
    m_InternalFormat = GraphicsFormat::None;
    resetProperties();
}

void Texture::destroy() {
    if (m_Texture) {
        m_Texture.destroy();
        updateMemUsage(0);
    }
}

void Texture::resetProperties() {
    setFilter(TextureDefaults::Filter);
    setWrapMode(TextureDefaults::Wrap);
    setAnisoLevel(TextureDefaults::Aniso);
}

void Texture::setFilter(TextureFilter filter) {
    bind();
    GLenum minFilter;

    if (filter == TextureFilter::Trilinear && !m_bHasMipmaps) {
        minFilter = minFilterToGL(TextureFilter::Bilinear);
    } else {
        minFilter = minFilterToGL(filter);
    }

    glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, magFilterToGL(filter));
    m_Filter = filter;
}

void Texture::setWrapMode(TextureWrapMode wrap) {
    setWrapModeS(wrap);
    setWrapModeT(wrap);
}

void Texture::setWrapModeS(TextureWrapMode wrap) {
    bind();
    glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, wrapToGL(wrap));
    m_WrapS = wrap;
}

void Texture::setWrapModeT(TextureWrapMode wrap) {
    bind();
    glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, wrapToGL(wrap));
    m_WrapT = wrap;
}

void Texture::setAnisoLevel(int level) {
    m_iAnisoLevel = level;

    if (GraphicsStack::get().isAnisoSupported()) {
        level = std::clamp(level, 1, GraphicsStack::get().getMaxAniso());
        bind();
        glTexParameterf(m_Target, GL_TEXTURE_MAX_ANISOTROPY, (float)level);
    }
}

GLuint Texture::getRenderTargetId() const {
    return getId();
}

GLenum Texture::getRenderTargetGLTarget() const {
    return getGLTarget();
}

void Texture::setDimension(GLenum target, TextureDimension dim) {
    m_Target = target;
    m_Dimension = dim;
}

void Texture::setInternalFormat(GraphicsFormat format) {
    m_InternalFormat = format;
}

void Texture::setSize(int wide, int tall) {
    m_iWide = wide;
    m_iTall = tall;
}

void Texture::setHasMipmaps(bool state) {
    m_bHasMipmaps = state;
}

void Texture::updateMemUsage(int64_t usage) {
    int64_t delta = usage - m_MemUsage.get();
    m_MemUsage.set(usage);
    m_siGlobalMemUsage += delta;
}

GLenum Texture::magFilterToGL(TextureFilter filter) {
    switch (filter) {
    case TextureFilter::Nearest:
        return GL_NEAREST;
    case TextureFilter::Bilinear:
    case TextureFilter::Trilinear:
        return GL_LINEAR;
    default:
        return 0;
    }
}

GLenum Texture::minFilterToGL(TextureFilter filter) {
    switch (filter) {
    case TextureFilter::Nearest:
        return GL_NEAREST;
    case TextureFilter::Bilinear:
        return GL_LINEAR;
    case TextureFilter::Trilinear:
        return GL_LINEAR_MIPMAP_LINEAR;
    default:
        return 0;
    }
}

GLenum Texture::wrapToGL(TextureWrapMode wrap) {
    switch (wrap) {
    case TextureWrapMode::Repeat:
        return GL_REPEAT;
    case TextureWrapMode::Clamp:
        return GL_CLAMP_TO_EDGE;
    case TextureWrapMode::Mirror:
        return GL_MIRRORED_REPEAT;
    default:
        return 0;
    }
}

GLenum Texture::graphicsFormatToGL(GraphicsFormat format) {
    switch (format) {
    case GraphicsFormat::None:
        return 0;
    case GraphicsFormat::R8:
        return GL_R8;
    case GraphicsFormat::R16F:
        return GL_R16F;
    case GraphicsFormat::RG8:
        return GL_RG8;
    case GraphicsFormat::RG16F:
        return GL_RG16F;
    case GraphicsFormat::RGB8:
        return GL_RGB8;
    case GraphicsFormat::RGBA8:
        return GL_RGBA8;
    case GraphicsFormat::RGB16F:
        return GL_RGB16F;
    case GraphicsFormat::RGBA16F:
        return GL_RGBA16F;
    case GraphicsFormat::SRGB8:
        return GL_SRGB8;
    case GraphicsFormat::SRGB8_ALPHA8:
        return GL_SRGB8_ALPHA8;
    case GraphicsFormat::Depth16:
        return GL_DEPTH_COMPONENT16;
    case GraphicsFormat::Depth24:
        return GL_DEPTH_COMPONENT24;
    case GraphicsFormat::Depth32:
        return GL_DEPTH_COMPONENT32;
    case GraphicsFormat::Depth24_Stencil8:
        return GL_DEPTH24_STENCIL8;
    default:
        return 0;
    }
}
