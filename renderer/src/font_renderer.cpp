#include <appfw/services.h>
#include <fmt/format.h>
#include <stdexcept>
#include <renderer/font_renderer.h>
#include <renderer/utils.h>

[[noreturn]] void fatalError(const std::string &msg);

static FontRenderer::FontShader s_FontShader;

//-----------------------------------------------------------------------------
// FontShader
//-----------------------------------------------------------------------------
FontRenderer::FontShader::FontShader()
    : BaseShader("FontShader")
    , m_ProjMat(this, "uProjection")
    , m_Color(this, "uTextColor")
    , m_Pos(this, "uPosition") {}

void FontRenderer::FontShader::create() {
    createProgram();
    createVertexShader("shaders/font/font.vert.glsl");
    createFragmentShader("shaders/font/font.frag.glsl");
    linkProgram();
}

void FontRenderer::FontShader::loadMatrices(glm::mat4 projMat) { m_ProjMat.set(projMat); }

void FontRenderer::FontShader::setColor(const glm::vec4 &c) { m_Color.set(c); }

void FontRenderer::FontShader::setPos(const glm::vec4 &pos) { m_Pos.set(pos); }

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
}

FontRenderer::~FontRenderer() {
    for (auto &i : m_CharInfo) {
        FontChar &fc = i.second;

        if (fc.m_Vao != 0) {
            glDeleteVertexArrays(1, &fc.m_Vao);
            fc.m_Vao = 0;
        }

        if (fc.m_Vbo != 0) {
            glDeleteBuffers(1, &fc.m_Vbo);
            fc.m_Vbo = 0;
        }
    }

    FT_Done_Face(m_FontFace);
}

void FontRenderer::updateViewportSize(int wide, int tall) {
    m_flTall = (float)tall;
    m_ProjMat = glm::ortho(0.0f, (float)wide, 0.0f, (float)tall);
}

float FontRenderer::drawChar(float x, float y, wchar_t c) {
    FontChar *fc = getChar(c);
    if (!fc)
        return 0;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(fc->m_Vao);

    GLfloat xpos = x + fc->m_Bearing.x;
    GLfloat ypos = (m_flTall - y) - (fc->m_Size.y - fc->m_Bearing.y);

    // Render glyph texture over quad
    glBindTexture(GL_TEXTURE_2D, fc->m_Texture);

    s_FontShader.enable();
    s_FontShader.setPos({xpos, ypos, 0.f, 0.f});

    // Render shadow
    s_FontShader.loadMatrices(glm::translate(m_ProjMat * glm::identity<glm::mat4>(), {1.f, -2.f, 0.f}));
    s_FontShader.setColor(colorToVec(m_ShadowColor));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Render quad
    s_FontShader.loadMatrices(m_ProjMat);
    s_FontShader.setColor(colorToVec(m_FontColor));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    s_FontShader.disable();

    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
    return (float)(fc->m_Advance >> 6); // Bitshift by 6 to get value in pixels (2^6 = 64)
}

void FontRenderer::setFontColor(appfw::Color color) { m_FontColor = color; }

void FontRenderer::setShadowColor(appfw::Color color) { m_ShadowColor = color; }

float FontRenderer::getCharWidth(wchar_t c) {
    FontChar *fc = getChar(c);
    if (!fc)
        return 0;
    return (float)(fc->m_Advance >> 6);
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

    // Size of glyph
    glm::ivec2 size = glm::ivec2(m_FontFace->glyph->bitmap.width, m_FontFace->glyph->bitmap.rows);

    // Offset from baseline to left/top of glyph
    glm::ivec2 bearing = glm::ivec2(m_FontFace->glyph->bitmap_left, m_FontFace->glyph->bitmap_top);

    // Offset to advance to next glyph
    GLuint advance = (GLuint)m_FontFace->glyph->advance.x;

    // Create VAO
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    GLfloat w = (GLfloat)size.x;
    GLfloat h = (GLfloat)size.y;

    GLfloat vertices[6][4] = {
        {0, 0 + h, 0.0, 0.0},
        {0, 0, 0.0, 1.0},
        {0 + w, 0, 1.0, 1.0},

        {0, 0 + h, 0.0, 0.0},
        {0 + w, 0, 1.0, 1.0},
        {0 + w, 0 + h, 1.0, 0.0}
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, false, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Now store character for later use
    FontChar character = {
        texture,
        size,
        bearing,
        advance,
        vao,
        vbo
    };

    return &(m_CharInfo.insert(std::pair<wchar_t, FontChar>(c, character)).first->second);
}
