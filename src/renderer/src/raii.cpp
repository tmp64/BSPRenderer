#include <renderer/raii.h>

//----------------------------------------------------------------
// GLTexture
//----------------------------------------------------------------
/*GLTexture::GLTexture(GLTexture &&other) noexcept {
    m_nId = other.m_nId;
    other.m_nId = 0;
}

GLTexture::~GLTexture() { destroy(); }

GLTexture &GLTexture::operator=(GLTexture &&other) noexcept {
    if (&other != this) {
        destroy();
        m_nId = other.m_nId;
        other.m_nId = 0;
    }

    return *this;
}

void GLTexture::create() noexcept {
    destroy();
    glGenTextures(1, &m_nId);
}

void GLTexture::destroy() noexcept {
    if (m_nId != 0) {
        glDeleteTextures(1, &m_nId);
        m_nId = 0;
    }
}*/

//----------------------------------------------------------------
// GLTextureVector
//----------------------------------------------------------------
/*GLTextureVector::GLTextureVector(GLTextureVector &&other) noexcept {
    m_nIds = std::move(other.m_nIds);
    other.m_nIds.clear();
}

GLTextureVector::~GLTextureVector() { destroy(); }

GLTextureVector &GLTextureVector::operator=(GLTextureVector &&other) noexcept {
    if (&other != this) {
        destroy();
        m_nIds = std::move(other.m_nIds);
        other.m_nIds.clear();
    }

    return *this;
}

void GLTextureVector::create(GLsizei count) {
    destroy();
    m_nIds.resize(count);
    glGenTextures(count, m_nIds.data());
}

void GLTextureVector::destroy() noexcept {
    if (!m_nIds.empty()) {
        glDeleteTextures((GLsizei)m_nIds.size(), m_nIds.data());
        m_nIds.clear();
    }
}*/
