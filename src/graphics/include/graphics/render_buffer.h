#ifndef GRAPHICS_RENDER_BUFFER_H
#define GRAPHICS_RENDER_BUFFER_H
#include <graphics/raii.h>
#include <graphics/vram_tracker.h>
#include <graphics/texture.h>

class RenderBuffer : public IRenderTarget {
public:
    RenderBuffer() = default;
    RenderBuffer(RenderBuffer &&) = default;
    ~RenderBuffer();
    RenderBuffer &operator=(RenderBuffer &&) = default;

    //! Creates an OpenGL renderbuffer name.
    void create(std::string_view name);

    //! Destroys the renderbuffer.
    void destroy();

    //! @returns whether the renderbuffer can be used for rendering.
    inline bool isValid() const { return m_InternalFormat != GraphicsFormat::None; }

    //! Binds the renderbuffer.
    inline void bind() const { glBindRenderbuffer(GL_RENDERBUFFER, m_Buf); }

    //! Initializes the renderbuffer.
    //! @param  internalFormat      Format in the GPU
    //! @param  wide                Width
    //! @param  tall                Height
    void init(GraphicsFormat internalFormat, int wide, int tall);

    //! @returns OpenGL buffer ID.
    inline GLuint getId() const { return m_Buf.id(); }

    //! @returns renderbuffer name.
    inline const std::string &getName() const { return m_Name; }

    //! @returns VRAM usage.
    inline int64_t getMemUsage() const { return m_MemUsage.get(); }

    inline GraphicsFormat getInternalFormat() const { return m_InternalFormat; }
    inline int getWide() const { return m_iWide; }
    inline int getTall() const { return m_iTall; }

    // IRenderTarget
    GLuint getRenderTargetId() const final override;
    GLenum getRenderTargetGLTarget() const final override;

    //! @returns VRAM used by all renderbuffers.
    static int64_t getGlobalMemUsage() { return m_siGlobalMemUsage; }

private:
    GLenum m_Target = 0;
    GLRenderbuffer m_Buf;
    VRAMTracker m_MemUsage;
    std::string m_Name;

    GraphicsFormat m_InternalFormat = GraphicsFormat::None;
    int m_iWide = 0;
    int m_iTall = 0;

    void updateMemUsage(int64_t usage);

    static inline int64_t m_siGlobalMemUsage = 0;
};

#endif
