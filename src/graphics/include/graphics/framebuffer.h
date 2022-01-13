#ifndef GRAPHICS_FRAMEBUFFER_H
#define GRAPHICS_FRAMEBUFFER_H
#include <glad/glad.h>
#include <graphics/raii.h>
#include <graphics/render_target.h>
#include <graphics/vram_tracker.h>

//! @see https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCheckFramebufferStatus.xhtml
enum class FramebufferStatus
{
    Complete,
    Undefined,
    IncompleteAttachment,
    MissingAttachment,
    Unsupported,
    IncompleteMultisample,
    IncompleteLayerTargets,
};

class Framebuffer {
public:
    static constexpr int MAX_COLOR_ATTACHMENTS = 8;

    //! Creates an OpenGL framebuffer name.
    void create(std::string_view name);

    //! Destroys the framebuffer, clearing the data.
    void destroy();

    //! @returns OpenGL framebuffer ID.
    inline GLuint getId() const { return m_Fb.id(); }

    //! Binds the framebuffer to specified target.
    //! Target can be GL_FRAMEBUFFER, GL_DRAW_FRAMEBUFFER,  GL_READ_FRAMEBUFFER.
    inline void bind(GLenum target) const { glBindFramebuffer(target, m_Fb); }

    //! @returns the status of the framebuffer.
    FramebufferStatus checkStatus() const;

    //! @returns whether the framebuffer is complete.
    bool isComplete() const;

    //! @returns the color buffer of specified attachment point
    inline IRenderTarget *getColorBuffer(int attachmentIdx) const {
        return m_pColorBuffers[attachmentIdx];
    }

    //! @returns the depth buffer
    inline IRenderTarget *getDepthBuffer() const { return m_pDepthBuffer; }

    //! @returns the combined depth and stencil buffer.
    inline IRenderTarget *getDepthStencilBuffer() const { return m_pDepthStencilBuffer; }

    //! Attaches a color buffer
    void attachColor(IRenderTarget *pTarget, int attachmentIdx);

    //! Attaches a depth buffer
    void attachDepth(IRenderTarget *pTarget);

    //! Attaches a depth-stencil buffer
    void attachDepthStencil(IRenderTarget *pTarget);

private:
    GLFramebuffer m_Fb;
    IRenderTarget *m_pColorBuffers[MAX_COLOR_ATTACHMENTS] = {nullptr};
    IRenderTarget *m_pDepthBuffer = nullptr;
    IRenderTarget *m_pDepthStencilBuffer = nullptr;
    std::string m_Name;

    void attachImage(GLenum attachment, IRenderTarget *pTarget);
};

#endif
