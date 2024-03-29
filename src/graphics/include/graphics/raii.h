#ifndef GRAPHICS_RAII_H
#define GRAPHICS_RAII_H
#include <vector>
#include <glad/glad.h>
#include <appfw/utils.h>

using GLGenFunction = void (*)(GLsizei n, GLuint *textures);
using GLDeleteFunction = void (*)(GLsizei n, const GLuint *textures);

//! Universal RAII wrapper for OpenGL objects.
//! Keeps an object.
template <GLGenFunction *GEN_FUNC, GLDeleteFunction *DEL_FUNC>
class GLRaiiWrapper {
public:
    GLRaiiWrapper() noexcept = default;
    GLRaiiWrapper(const GLRaiiWrapper &) = delete;

    GLRaiiWrapper(GLRaiiWrapper &&other) noexcept {
        m_nId = other.m_nId;
        other.m_nId = 0;
    }

    ~GLRaiiWrapper() { destroy(); }

    GLRaiiWrapper &operator=(const GLRaiiWrapper &) = delete;

    GLRaiiWrapper &operator=(GLRaiiWrapper &&other) {
        if (&other != this) {
            destroy();
            m_nId = other.m_nId;
            other.m_nId = 0;
        }

        return *this;
    }

    inline GLuint id() const { return m_nId; }

    inline operator GLuint() const { return m_nId; }

    void create() noexcept {
        destroy();
        (*GEN_FUNC)(1, &m_nId);
    }

    void destroy() noexcept {
        if (m_nId != 0) {
            (*DEL_FUNC)(1, &m_nId);
            m_nId = 0;
        }
    }

public:
    GLuint m_nId = 0;
};

//! Universal RAII wrapper for OpenGL objects.
//! Keeps an array of objects.
template <GLGenFunction *GEN_FUNC, GLDeleteFunction *DEL_FUNC>
class GLRaiiVectorWrapper : appfw::NoCopy {
public:
    GLRaiiVectorWrapper() noexcept = default;

    GLRaiiVectorWrapper(GLRaiiVectorWrapper &&other) noexcept {
        m_nIds = std::move(other.m_nIds);
        other.m_nIds.clear();
    }

    ~GLRaiiVectorWrapper() { destroy(); }

    GLRaiiVectorWrapper &operator=(GLRaiiVectorWrapper &&other) noexcept {
        if (&other != this) {
            destroy();
            m_nIds = std::move(other.m_nIds);
            other.m_nIds.clear();
        }

        return *this;
    }

    void create(GLsizei count) {
        destroy();
        m_nIds.resize(count);
        (*GEN_FUNC)(count, m_nIds.data());
    }

    void destroy() noexcept {
        if (!m_nIds.empty()) {
            (*DEL_FUNC)((GLsizei)m_nIds.size(), m_nIds.data());
            m_nIds.clear();
        }
    }

    inline GLuint id(size_t idx) { return m_nIds[idx]; }
    inline GLsizei size() { return (GLsizei)m_nIds.size(); }

public:
    std::vector<GLuint> m_nIds;
};

//! RAII wrapper for one OpenGL texture.
using GLTexture = GLRaiiWrapper<&glGenTextures, &glDeleteTextures>;

//! RAII wrapper for a number of OpenGL textures.
using GLTextureVector = GLRaiiVectorWrapper<&glGenTextures, &glDeleteTextures>;

//! RAII wrapper for a VAO (vertex array object).
using GLVao = GLRaiiWrapper<&glGenVertexArrays, &glDeleteVertexArrays>;

//! RAII wrapper for an OpenGL buffer.
using GLBuffer = GLRaiiWrapper<&glGenBuffers, &glDeleteBuffers>;

//! RAII wrapper for a framebuffer.
using GLFramebuffer = GLRaiiWrapper<&glGenFramebuffers, &glDeleteFramebuffers>;

//! RAII wrapper for a renderbuffer.
using GLRenderbuffer = GLRaiiWrapper<&glGenRenderbuffers, &glDeleteRenderbuffers>;

#endif
