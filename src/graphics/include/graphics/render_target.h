#ifndef GRAPHICS_RENDER_TARGET_H
#define GRAPHICS_RENDER_TARGET_H
#include <glad/glad.h>

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;

    //! @returns the OpenGL id of the target
    virtual GLuint getRenderTargetId() const = 0;

    //! @returns the OpenGL target (e.g. GL_TEXTURE_2D or GL_RENDERBUFFER)
    virtual GLenum getRenderTargetGLTarget() const = 0;
};

#endif
