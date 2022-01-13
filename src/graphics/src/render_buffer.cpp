#include <graphics/render_buffer.h>
#include <graphics/texture2d.h>

RenderBuffer::~RenderBuffer() {
    destroy();
}

void RenderBuffer::create(std::string_view name) {
    m_Buf.create();
    m_Name = name;
}

void RenderBuffer::destroy() {
    if (m_Buf) {
        m_Buf.destroy();
        updateMemUsage(0);
    }
}

void RenderBuffer::init(GraphicsFormat internalFormat, int wide, int tall) {
    AFW_ASSERT(!isValid());
    m_InternalFormat = internalFormat;
    m_iWide = wide;
    m_iTall = tall;

    bind();
    glRenderbufferStorage(GL_RENDERBUFFER, Texture::graphicsFormatToGL(internalFormat), wide, tall);
    updateMemUsage(Texture2D::calcTextureMemUsage(internalFormat, wide, tall, false));
}

GLuint RenderBuffer::getRenderTargetId() const {
    return getId();
}

GLenum RenderBuffer::getRenderTargetGLTarget() const {
    return GL_RENDERBUFFER;
}

void RenderBuffer::updateMemUsage(int64_t usage) {
    int64_t delta = usage - m_MemUsage.get();
    m_MemUsage.set(usage);
    m_siGlobalMemUsage += delta;
}
