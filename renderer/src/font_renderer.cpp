#include <appfw/services.h>
#include <fmt/format.h>
#include <stdexcept>
#include <renderer/font_renderer.h>
#include <renderer/utils.h>

[[noreturn]] void fatalError(const std::string &msg);

static FontRenderer::FontShader s_FontShader;
static FontRenderer::ShadowShader s_ShadowShader;

//-----------------------------------------------------------------------------
// FontShader
//-----------------------------------------------------------------------------
FontRenderer::FontShader::FontShader()
    : BaseShader("FontShader"), m_ProjMat(this, "projection"), m_Color(this, "textColor") {}

void FontRenderer::FontShader::create() {
    createProgram();
    createVertexShader("shaders/font.vert.glsl");
    createFragmentShader("shaders/font.frag.glsl");
    linkProgram();
}

void FontRenderer::FontShader::loadMatrices(glm::mat4 projMat) { m_ProjMat.set(projMat); }

void FontRenderer::FontShader::setColor(const glm::vec4 &c) { m_Color.set(c); }

//-----------------------------------------------------------------------------
// ShadowShader
//-----------------------------------------------------------------------------
FontRenderer::ShadowShader::ShadowShader()
    : BaseShader("FontShadowShader"), m_ProjMat(this, "projection"), m_Color(this, "textColor") {}

void FontRenderer::ShadowShader::create() {
    createProgram();
    createVertexShader("shaders/font_shadow.vert.glsl");
    createFragmentShader("shaders/font_shadow.frag.glsl");
    linkProgram();
}

void FontRenderer::ShadowShader::loadMatrices(glm::mat4 projMat) { m_ProjMat.set(projMat); }

void FontRenderer::ShadowShader::setColor(const glm::vec4 &c) { m_Color.set(c); }

//-----------------------------------------------------------------------------
// FontRenderer
//-----------------------------------------------------------------------------
FontRenderer::FontRenderer(const char *fontPath, int fontSize) {
    if (!m_sFreeType) {
        if (FT_Init_FreeType(&m_sFreeType)) {
            fatalError("Failed to initialize FreeType");
        }
    }

    if (FT_New_Face(m_sFreeType, fontPath, 0, &m_FontFace)) {
        throw std::runtime_error(std::string("Failed to load font ") + fontPath);
    }

    FT_Set_Pixel_Sizes(m_FontFace, 0, fontSize);

    glGenVertexArrays(1, &m_Vao);
    glGenBuffers(1, &m_Vbo);
    glBindVertexArray(m_Vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, false, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

FontRenderer::~FontRenderer() {
    FT_Done_Face(m_FontFace);
}

void FontRenderer::updateViewportSize(int wide, int tall) {
    m_flTall = (float)tall;
    m_ProjMat = glm::ortho(0.0f, (float)wide, 0.0f, (float)tall);
}

float FontRenderer::drawChar(float x, float y, wchar_t c, float scale) {
    FontChar *fc = getChar(c);
    if (!fc)
        return 0;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_Vao);

    GLfloat xpos = x + fc->m_Bearing.x * scale;
    GLfloat ypos = (m_flTall - y) - (fc->m_Size.y - fc->m_Bearing.y) * scale;

    GLfloat w = fc->m_Size.x * scale;
    GLfloat h = fc->m_Size.y * scale;
    // Update VBO for each character
    GLfloat vertices[6][4] = {{xpos, ypos + h, 0.0, 0.0}, {xpos, ypos, 0.0, 1.0},     {xpos + w, ypos, 1.0, 1.0},

                              {xpos, ypos + h, 0.0, 0.0}, {xpos + w, ypos, 1.0, 1.0}, {xpos + w, ypos + h, 1.0, 0.0}};

    // Render glyph texture over quad
    glBindTexture(GL_TEXTURE_2D, fc->m_Texture);

    // Update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Render shadow
    s_ShadowShader.enable();
    s_ShadowShader.loadMatrices(glm::translate(m_ProjMat * glm::identity<glm::mat4>(), {1.f, -2.f, 0.f}));
    s_ShadowShader.setColor(colorToVec(m_ShadowColor));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    s_ShadowShader.disable();

    // Render quad
    s_FontShader.enable();
    s_FontShader.loadMatrices(m_ProjMat);
    s_FontShader.setColor(colorToVec(m_FontColor));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    s_FontShader.disable();

    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
    return (fc->m_Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
}

void FontRenderer::setFontColor(appfw::Color color) { m_FontColor = color; }

void FontRenderer::setShadowColor(appfw::Color color) { m_ShadowColor = color; }

float FontRenderer::getCharWidth(wchar_t c, float scale) {
    FontChar *fc = getChar(c);
    if (!fc)
        return 0;
    return (fc->m_Advance >> 6) * scale;
}

FontRenderer::FontChar *FontRenderer::getChar(wchar_t c) {
    auto it = m_CharInfo.find(c);
    if (it != m_CharInfo.end())
        return &it->second;

    if (FT_Load_Char(m_FontFace, c, FT_LOAD_RENDER)) {
        logError("FontRenderer: Failed to load glyph {} ({})", (char)c, static_cast<int>(c));
        return nullptr;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Generate texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_FontFace->glyph->bitmap.width, m_FontFace->glyph->bitmap.rows, 0,
                   GL_RED, GL_UNSIGNED_BYTE, m_FontFace->glyph->bitmap.buffer);

    // Set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Now store character for later use
    FontChar character = {texture, glm::ivec2(m_FontFace->glyph->bitmap.width, m_FontFace->glyph->bitmap.rows),
                             glm::ivec2(m_FontFace->glyph->bitmap_left, m_FontFace->glyph->bitmap_top),
                             (GLuint)m_FontFace->glyph->advance.x};

    return &(m_CharInfo.insert(std::pair<wchar_t, FontChar>(c, character)).first->second);
}
