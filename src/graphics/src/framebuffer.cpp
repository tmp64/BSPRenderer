#include <graphics/framebuffer.h>

void Framebuffer::create(std::string_view name) {
    m_Fb.create();
    m_Name = name;
}

void Framebuffer::destroy() {
    m_Fb.destroy();

    // Forget all attachments
    std::fill(std::begin(m_pColorBuffers), std::end(m_pColorBuffers), nullptr);
    m_pDepthBuffer = nullptr;
    m_pDepthStencilBuffer = nullptr;
}

FramebufferStatus Framebuffer::checkStatus() const {
    GLenum status = glCheckFramebufferStatus(m_Fb);

    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
        return FramebufferStatus::Complete;
    case GL_FRAMEBUFFER_UNDEFINED:
        return FramebufferStatus::Undefined;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return FramebufferStatus::IncompleteAttachment;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return FramebufferStatus::MissingAttachment;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return FramebufferStatus::Unsupported;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return FramebufferStatus::IncompleteMultisample;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        return FramebufferStatus::IncompleteLayerTargets;
    default:
        return FramebufferStatus::Unsupported;
    }
}

bool Framebuffer::isComplete() const {
    bind(GL_FRAMEBUFFER);
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void Framebuffer::attachColor(IRenderTarget *pTarget, int attachmentIdx) {
    AFW_ASSERT(attachmentIdx >= 0 && attachmentIdx < MAX_COLOR_ATTACHMENTS);
    attachImage(GL_COLOR_ATTACHMENT0 + attachmentIdx, pTarget);
    m_pColorBuffers[attachmentIdx] = pTarget;
}

void Framebuffer::attachDepth(IRenderTarget *pTarget) {
    AFW_ASSERT(!m_pDepthStencilBuffer);
    attachImage(GL_DEPTH_ATTACHMENT, pTarget);
    m_pDepthBuffer = pTarget;
}

void Framebuffer::attachDepthStencil(IRenderTarget *pTarget) {
    AFW_ASSERT(!m_pDepthBuffer);
    attachImage(GL_DEPTH_STENCIL_ATTACHMENT, pTarget);
    m_pDepthStencilBuffer = pTarget;
}

void Framebuffer::attachImage(GLenum attachment, IRenderTarget *pTarget) {
    bind(GL_FRAMEBUFFER);

    if (!pTarget) {
        // Detach it
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, 0);
        return;
    }

    switch (pTarget->getRenderTargetGLTarget()) {
    case GL_TEXTURE_2D:
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D,
                               pTarget->getRenderTargetId(), 0);
        break;
    case GL_RENDERBUFFER:
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER,
                                  pTarget->getRenderTargetId());
        break;
    default:
        AFW_ASSERT(false);
        throw std::logic_error("Invalid render target");
    }
}
