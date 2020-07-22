#ifndef FONT_RENDERER_H
#define FONT_RENDERER_H
#include <unordered_map>
#include <appfw/utils.h>
#include <appfw/color.h>
#include <renderer/base_shader.h>
#include <ft2build.h>
#include FT_FREETYPE_H

class FontRenderer : appfw::utils::NoCopy {
public:
    class FontShader : public BaseShader {
    public:
        FontShader();
        virtual void create() override;
        void loadMatrices(glm::mat4 projMat);
        void setColor(const glm::vec4 &c);

    private:
        ShaderUniform<glm::mat4> m_ProjMat;
        ShaderUniform<glm::vec4> m_Color;
    };

    class ShadowShader : public BaseShader {
    public:
        ShadowShader();
        virtual void create() override;
        void loadMatrices(glm::mat4 projMat);
        void setColor(const glm::vec4 &c);

    private:
        ShaderUniform<glm::mat4> m_ProjMat;
        ShaderUniform<glm::vec4> m_Color;
    };

    /**
     * Creates a font renderer. Can only be called after OpenGL is initialized.
     * @param   fontPath    Path to .ttf file
     * @param   fontSize    Size of the font
     */
    FontRenderer(const char *fontPath, int fontSize);

    /**
     * Destructor of FontRenderer
     */
    ~FontRenderer();

    /**
     * Must be called when viewport size has changed.
     * @param   wide    New width of the viewport
     * @param   tall    New height of the viewport
     */
    void updateViewportSize(int wide, int tall);

    /**
     * Draws a character at specified coordinates
     * @return Offset to next character
     */
    float drawChar(float x, float y, wchar_t c, float scale = 1.0f);

    /**
     * Sets color of the text.
     */
    void setFontColor(appfw::Color color);

    /**
     * Sets color of the shadow.
     */
    void setShadowColor(appfw::Color color);

    /**
     * Returns withd of a char.
     */
    float getCharWidth(wchar_t c, float scale = 1.f);

private:
    struct FontChar {
        GLuint m_Texture;     // ID handle of the glyph texture
        glm::ivec2 m_Size;    // Size of glyph
        glm::ivec2 m_Bearing; // Offset from baseline to left/top of glyph
        GLuint m_Advance;     // Offset to advance to next glyph
    };

    FT_Face m_FontFace = nullptr;
    std::unordered_map<wchar_t, FontChar> m_CharInfo;
    GLuint m_Vao = 0, m_Vbo = 0;
    glm::mat4 m_ProjMat = glm::mat4();
    appfw::Color m_FontColor = appfw::Color(255, 255, 255, 255);
    appfw::Color m_ShadowColor = appfw::Color(0, 0, 0, 200);
    float m_flTall = 0;

    FontChar *getChar(wchar_t c);

    static inline FT_Library m_sFreeType = nullptr;
};

#endif
