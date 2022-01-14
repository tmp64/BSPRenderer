#ifndef GRAPHICS_TEXTURE_H
#define GRAPHICS_TEXTURE_H
#include <graphics/raii.h>
#include <graphics/vram_tracker.h>
#include <graphics/render_target.h>

enum class TextureDimension
{
    None,
    Tex2D,
    Cube,
};

enum class TextureFilter
{
    Nearest,    //!< No filtering, takes nearest sample.
    Bilinear,   //!< Linear filtering between samples.
    Trilinear,  //!< Linear filtering between samples and between mipmap levels.
};

enum class TextureWrapMode
{
    Repeat,     //!< Tiles the texture, creating a repeating pattern.
    Clamp,      //!< Clamps the texture to the last pixel at the edge.
    Mirror,     //!< Tiles the texture, creating a repeating pattern by mirroring it at every integer boundary.
};

enum class GraphicsFormat
{
    None,
    R8,
    R16F,
    RG8,
    RG16F,
    RGB8,
    RGBA8,
    RGB16F,
    RGBA16F,
    SRGB8,
    SRGB8_ALPHA8,
    Depth16,
    Depth24,
    Depth32,
    Depth24_Stencil8
};

struct TextureDefaults {
    static constexpr TextureFilter Filter = TextureFilter::Nearest;
    static constexpr TextureWrapMode Wrap = TextureWrapMode::Repeat;
    static constexpr int Aniso = 0;
};

//! Base class for different texture types.
//! Most methods change currently bound texture, be aware of that.
class Texture : public IRenderTarget {
public:
    Texture() = default;
    Texture(Texture &&) = default;
    virtual ~Texture();

    Texture &operator=(Texture &&) = default;

    //! Creates an OpenGL texture name
    void create(std::string_view name);

    //! Destroys the OpenGL texture.
    virtual void destroy();

    //! Binds the texture.
    inline void bind() const { glBindTexture(m_Target, m_Texture); }

    //! @returns whether the texture can be used for rendering.
    inline bool isValid() const { return m_InternalFormat != GraphicsFormat::None; }

    //! Resets filtering and wrapping properties to defaults.
    virtual void resetProperties();

    inline GLuint getId() const { return m_Texture; }
    inline GLenum getGLTarget() const { return m_Target; }
    inline TextureDimension getDimension() const { return m_Dimension; }
    inline GraphicsFormat getInternalFormat() const { return m_InternalFormat; }
    inline int getWide() const { return m_iWide; }
    inline int getTall() const { return m_iTall; }
    inline bool hasMipmaps() const { return m_bHasMipmaps; }
    inline TextureFilter getFilter() const { return m_Filter; }
    void setFilter(TextureFilter filter);
    inline TextureWrapMode getWrapModeS() const { return m_WrapS; }
    inline TextureWrapMode getWrapModeT() const { return m_WrapT; }
    virtual void setWrapMode(TextureWrapMode wrap);
    void setWrapModeS(TextureWrapMode wrap);
    void setWrapModeT(TextureWrapMode wrap);
    inline int getAnisoLevel() const { return m_iAnisoLevel; }
    void setAnisoLevel(int level);

    // IRenderTarget
    GLuint getRenderTargetId() const final override;
    GLenum getRenderTargetGLTarget() const final override;

    //! @returns VRAM used by all textures.
    static inline int64_t getGlobalMemUsage() { return m_siGlobalMemUsage; }

    static GLenum magFilterToGL(TextureFilter filter);
    static GLenum minFilterToGL(TextureFilter filter);
    static GLenum wrapToGL(TextureWrapMode wrap);
    static GLenum graphicsFormatToGL(GraphicsFormat format);

protected:
    void setDimension(GLenum target, TextureDimension dim);
    void setInternalFormat(GraphicsFormat format);
    void setSize(int wide, int tall);
    void setHasMipmaps(bool state);
    void updateMemUsage(int64_t usage);

private:
    GLTexture m_Texture;
    VRAMTracker m_MemUsage;
    GLenum m_Target = 0;
    TextureDimension m_Dimension = TextureDimension::None;
    GraphicsFormat m_InternalFormat = GraphicsFormat::None;
    int m_iWide = 0;
    int m_iTall = 0;
    bool m_bHasMipmaps = false;

    TextureFilter m_Filter = TextureDefaults::Filter;
    TextureWrapMode m_WrapS = TextureDefaults::Wrap;
    TextureWrapMode m_WrapT = TextureDefaults::Wrap;
    int m_iAnisoLevel = TextureDefaults::Aniso;

    std::string m_Name;

    static inline int64_t m_siGlobalMemUsage = 0;
};

#endif
