#include "font_renderer.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace fontrndr
{
	struct font_char_t {
		GLuint     m_Texture;	// ID handle of the glyph texture
		glm::ivec2 m_Size;		// Size of glyph
		glm::ivec2 m_Bearing;	// Offset from baseline to left/top of glyph
		GLuint     m_Advance;	// Offset to advance to next glyph
	};

	class CFontShader : public CBaseShader
	{
	public:
		CFontShader();
		virtual void Init();
		void LoadMatrices();
		void SetColor(const glm::vec4 &c);

	private:
		CUniform<glm::mat4> m_ProjMat;
		CUniform<glm::vec4> m_Color;
	};

	class CShadowShader : public CBaseShader
	{
	public:
		CShadowShader();
		virtual void Init();
		void LoadMatrices();
		void SetColor(const glm::vec4 &c);

	private:
		CUniform<glm::mat4> m_ProjMat;
		CUniform<glm::vec4> m_Color;
	};

	static FT_Library g_FreeType = NULL;
	static FT_Face g_FontFace;
	static std::unordered_map<wchar_t, font_char_t> g_CharInfo;
	static GLuint g_Vao, g_Vbo;
	static glm::mat4 g_ProjMat;
	static CFontShader g_FontShader;
	static CShadowShader g_ShadowShader;
	static glm::vec4 g_FontColor = { 1.0f, 1.f, 0.f, 1.f }, g_ShadowColor = { 0.0f, 0.f, 0.f, 0.8f };

	static font_char_t *GetChar(wchar_t c);
}

//-----------------------------------------------------------------------------
// CFontShader
//-----------------------------------------------------------------------------
fontrndr::CFontShader::CFontShader() : CBaseShader("FontShader"),
m_ProjMat(this, "projection"),
m_Color(this, "textColor")
{

}

void fontrndr::CFontShader::Init()
{
	CreateProgram();
	CreateVertexShader("shaders/font.vert.glsl");
	CreateFragmentShader("shaders/font.frag.glsl");
	LinkProgram();
	CheckGLError();
}

void fontrndr::CFontShader::LoadMatrices()
{
	m_ProjMat.Set(g_ProjMat);
}


void fontrndr::CFontShader::SetColor(const glm::vec4 &c)
{
	m_Color.Set(c);
}

//-----------------------------------------------------------------------------
// CShadowShader
//-----------------------------------------------------------------------------
fontrndr::CShadowShader::CShadowShader() : CBaseShader("FontShadowShader"),
m_ProjMat(this, "projection"),
m_Color(this, "textColor")
{

}

void fontrndr::CShadowShader::Init()
{
	CreateProgram();
	CreateVertexShader("shaders/font_shadow.vert.glsl");
	CreateFragmentShader("shaders/font_shadow.frag.glsl");
	LinkProgram();
	CheckGLError();
}

void fontrndr::CShadowShader::LoadMatrices()
{
	m_ProjMat.Set(g_ProjMat);
}


void fontrndr::CShadowShader::SetColor(const glm::vec4 &c)
{
	m_Color.Set(c);
}

//-----------------------------------------------------------------------------
// FontRenderer
//-----------------------------------------------------------------------------
void fontrndr::Initialize()
{
	if (FT_Init_FreeType(&g_FreeType))
		std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

	if (FT_New_Face(g_FreeType, "fonts/OpenSans-Semibold.ttf", 0, &g_FontFace))
		std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;

	FT_Set_Pixel_Sizes(g_FontFace, 0, 18);

	
	gl::GenVertexArrays(1, &g_Vao);
	gl::GenBuffers(1, &g_Vbo);
	gl::BindVertexArray(g_Vao);
	gl::BindBuffer(gl::ARRAY_BUFFER, g_Vbo);
	gl::BufferData(gl::ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, gl::DYNAMIC_DRAW);
	gl::EnableVertexAttribArray(0);
	gl::VertexAttribPointer(0, 4, gl::FLOAT, false, 4 * sizeof(GLfloat), 0);
	gl::BindBuffer(gl::ARRAY_BUFFER, 0);
	gl::BindVertexArray(0);

	sf::Vector2u winSize = gMainWindow->getSize();
	g_ProjMat = glm::ortho(0.0f, (float)winSize.x, 0.0f, (float)winSize.y);

	g_FontShader.Init();
	g_FontShader.Enable();
	g_FontShader.LoadMatrices();
	g_FontShader.SetColor(g_FontColor);
	g_FontShader.Disable();

	g_ProjMat = glm::ortho(0.0f, (float)winSize.x, 0.0f, (float)winSize.y);
	g_ProjMat = glm::translate(g_ProjMat, { 1.5f, -2.0f, 0 });

	g_ShadowShader.Init();
	g_ShadowShader.Enable();
	g_ShadowShader.LoadMatrices();
	g_ShadowShader.SetColor(g_ShadowColor);
	g_ShadowShader.Disable();
}

void fontrndr::Shutdown()
{
	FT_Done_Face(g_FontFace);
	FT_Done_FreeType(g_FreeType);
	g_FontShader.Deinit();
	g_ShadowShader.Deinit();
}

static fontrndr::font_char_t *fontrndr::GetChar(wchar_t c)
{
	auto it = g_CharInfo.find(c);
	if (it != g_CharInfo.end())
		return &it->second;

	if (FT_Load_Char(g_FontFace, c, FT_LOAD_RENDER))
	{
		std::cerr << "ERROR::FREETYTPE: Failed to load Glyph" << c << " (" << static_cast<int>(c) << ")" << std::endl;
		return nullptr;
	}

	gl::PixelStorei(gl::UNPACK_ALIGNMENT, 1);

	// Generate texture
	GLuint texture;
	gl::GenTextures(1, &texture);
	gl::BindTexture(gl::TEXTURE_2D, texture);
	gl::TexImage2D(
		gl::TEXTURE_2D,
		0,
		gl::RED,
		g_FontFace->glyph->bitmap.width,
		g_FontFace->glyph->bitmap.rows,
		0,
		gl::RED,
		gl::UNSIGNED_BYTE,
		g_FontFace->glyph->bitmap.buffer
	);

	// Set texture options
	gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
	gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);
	gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
	gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

	// Now store character for later use
	font_char_t character = {
		texture,
		glm::ivec2(g_FontFace->glyph->bitmap.width, g_FontFace->glyph->bitmap.rows),
		glm::ivec2(g_FontFace->glyph->bitmap_left, g_FontFace->glyph->bitmap_top),
		g_FontFace->glyph->advance.x
	};

	return &(g_CharInfo.insert(std::pair<wchar_t, font_char_t>(c, character)).first->second);
}

int fontrndr::DrawChar(int x, int y, wchar_t c, float scale)
{
	font_char_t *fc = GetChar(c);
	if (!fc)
		return 0;

	//gl::Enable(gl::BLEND);
	//gl::BlendFunc(gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
	gl::ActiveTexture(gl::TEXTURE0);
	gl::BindVertexArray(g_Vao);

	GLfloat xpos = x + fc->m_Bearing.x * scale;
	GLfloat ypos = y - (fc->m_Size.y - fc->m_Bearing.y) * scale;

	GLfloat w = fc->m_Size.x * scale;
	GLfloat h = fc->m_Size.y * scale;
	// Update VBO for each character
	GLfloat vertices[6][4] = {
		{ xpos,     ypos + h,   0.0, 0.0 },
		{ xpos,     ypos,       0.0, 1.0 },
		{ xpos + w, ypos,       1.0, 1.0 },

		{ xpos,     ypos + h,   0.0, 0.0 },
		{ xpos + w, ypos,       1.0, 1.0 },
		{ xpos + w, ypos + h,   1.0, 0.0 }
	};

	// Render glyph texture over quad
	gl::BindTexture(gl::TEXTURE_2D, fc->m_Texture);

	// Update content of VBO memory
	gl::BindBuffer(gl::ARRAY_BUFFER, g_Vbo);
	gl::BufferSubData(gl::ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	gl::BindBuffer(gl::ARRAY_BUFFER, 0);

	// Render shadow
	g_ShadowShader.Enable();
	gl::DrawArrays(gl::TRIANGLES, 0, 6);
	g_ShadowShader.Disable();

	// Render quad
	g_FontShader.Enable();
	gl::DrawArrays(gl::TRIANGLES, 0, 6);
	g_FontShader.Disable();

	//gl::Disable(gl::BLEND);
	gl::BindVertexArray(0);
	gl::BindTexture(gl::TEXTURE_2D, 0);

	// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
	return (fc->m_Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
}
