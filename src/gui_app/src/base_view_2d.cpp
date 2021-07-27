#include <renderer/base_shader.h>
#include <gui_app/base_view_2d.h>

//-----------------------------------------------------------
// GridShader
//-----------------------------------------------------------
class BaseView2D::TextureShader : public BaseShader {
public:
    TextureShader()
        : BaseShader()
        , m_ViewMat(this, "uViewMatrix")
        , m_ProjMat(this, "uProjMatrix") {
        setTitle("BaseView2D_TextureShader");
        setVert("assets:shaders/base_view_2d/texture.vert");
        setFrag("assets:shaders/base_view_2d/texture.frag");
    }

    void loadMatrices(const BaseView2D &view) {
        m_ViewMat.set(view.getViewMat());
        m_ProjMat.set(view.getProjMat());
    }

    static BaseView2D::TextureShader instance;

private:
    ShaderUniform<glm::mat4> m_ViewMat, m_ProjMat;
};

BaseView2D::TextureShader BaseView2D::TextureShader::instance;

//-----------------------------------------------------------
// BaseView2D
//-----------------------------------------------------------
BaseView2D::BaseView2D() {
	// Create texture quad
    glGenVertexArrays(1, &m_nQuadVao);
    glGenBuffers(1, &m_nQuadVbo);

    glBindVertexArray(m_nQuadVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_nQuadVbo);

    static_assert(sizeof(TexVertex) == 5 * sizeof(float), "TexVertex is not packed");
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(TexVertex), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(TexVertex), (void *)offsetof(TexVertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(TexVertex), (void *)offsetof(TexVertex, tex));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void BaseView2D::setProjBounds(float left, float right, float bottom, float top) {
	m_flLeft = left;
    m_flRight = right;
	m_flBottom = bottom;
    m_flTop = top;
}

glm::mat4 BaseView2D::getProjMat() const { return glm::ortho(m_flLeft, m_flRight, m_flBottom, m_flTop, -1.f, 1.f); }

glm::mat4 BaseView2D::getViewMat() const { return glm::identity<glm::mat4>(); }

void BaseView2D::drawTexture(GLuint texture, glm::vec2 pos, glm::vec2 size, glm::vec2 uv0, glm::vec2 uv1) {
    TexVertex vertexData[4] = {
        {glm::vec3(pos, 0.f), uv0},
        {glm::vec3(pos.x + size.x, pos.y, 0.f), glm::vec2(uv1.x, uv0.y)},
        {glm::vec3(pos.x, pos.y + size.y, 0.f), glm::vec2(uv0.x, uv1.y)},
        {glm::vec3(pos + size, 0.f), uv1},
    };

    TextureShader::instance.enable();
    TextureShader::instance.loadMatrices(*this);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindVertexArray(m_nQuadVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_nQuadVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertexData), vertexData);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);

    TextureShader::instance.disable();
}
