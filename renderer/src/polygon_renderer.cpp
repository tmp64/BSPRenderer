#include <appfw/services.h>
#include <renderer/polygon_renderer.h>

static PolygonRenderer::Shader s_Shader;

//-----------------------------------------------------------------------------
// Shader
//-----------------------------------------------------------------------------
PolygonRenderer::Shader::Shader()
    : BaseShader("PolygonRendererShader")
    , m_ViewMat(this, "uViewMatrix")
    , m_ProjMat(this, "uProjMatrix")
    , m_Color(this, "uColor") {}

void PolygonRenderer::Shader::create() {
    createProgram();
    createVertexShader("shaders/PolygonRendererShader.vert.glsl");
    createFragmentShader("shaders/PolygonRendererShader.frag.glsl");
    linkProgram();
}

void PolygonRenderer::Shader::loadMatrices(const BaseRenderer::DrawOptions &options) {
    glm::mat4 proj =
        glm::perspective(glm::radians(options.fov), options.aspect, options.near, options.far);
    m_ProjMat.set(proj);

    // From quake
    glm::mat4 viewMat = glm::identity<glm::mat4>();
    viewMat = glm::rotate(viewMat, glm::radians(-90.f), {1.0f, 0.0f, 0.0f});
    viewMat = glm::rotate(viewMat, glm::radians(90.f), {0.0f, 0.0f, 1.0f});
    viewMat = glm::rotate(viewMat, glm::radians(-options.viewAngles.z), {1.0f, 0.0f, 0.0f});
    viewMat = glm::rotate(viewMat, glm::radians(-options.viewAngles.x), {0.0f, 1.0f, 0.0f});
    viewMat = glm::rotate(viewMat, glm::radians(-options.viewAngles.y), {0.0f, 0.0f, 1.0f});
    viewMat = glm::translate(viewMat, {-options.viewOrigin.x, -options.viewOrigin.y, -options.viewOrigin.z});
    m_ViewMat.set(viewMat);
}

void PolygonRenderer::Shader::setColor(const glm::vec3 &c) { m_Color.set(c); }

//-----------------------------------------------------------------------------
// Surface
//-----------------------------------------------------------------------------
PolygonRenderer::Surface::Surface(const LevelSurface &baseSurface) noexcept {
    glGenVertexArrays(1, &m_Vao);
    glGenBuffers(1, &m_Vbo);

    glBindVertexArray(m_Vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);

    m_VertexCount = baseSurface.vertices.size();
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * m_VertexCount, baseSurface.vertices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    auto fnGetRandColor = []() { return (rand() % 256) / 255.f; };
    m_Color = {fnGetRandColor(), fnGetRandColor(), fnGetRandColor()};
}

PolygonRenderer::Surface::Surface(Surface &&other) noexcept {
    m_Vao = other.m_Vao;
    m_Vbo = other.m_Vbo;
    m_Color = other.m_Color;
    m_VertexCount = other.m_VertexCount;

    other.m_Vao = 0;
    other.m_Vbo = 0;
    other.m_VertexCount = 0;
}

PolygonRenderer::Surface::~Surface() {
    if (m_Vao != 0) {
        glDeleteVertexArrays(1, &m_Vao);
    }

    if (m_Vbo != 0) {
        glDeleteBuffers(1, &m_Vbo);
    }
}

void PolygonRenderer::Surface::draw() noexcept {
    glBindVertexArray(m_Vao);

    s_Shader.setColor(m_Color);
    glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)m_VertexCount);

    glBindVertexArray(0);
}

//-----------------------------------------------------------------------------
// PolygonRenderer
//-----------------------------------------------------------------------------
void PolygonRenderer::createSurfaces() {
    AFW_ASSERT(m_Surfaces.empty());
    m_Surfaces.reserve(m_BaseSurfaces.size());

    for (size_t i = 0; i < m_BaseSurfaces.size(); i++) {
        const LevelSurface &baseSurface = m_BaseSurfaces[i];
        m_Surfaces.emplace_back(baseSurface);
    }
}

void PolygonRenderer::destroySurfaces() {
    m_Surfaces.clear();
    m_Surfaces.shrink_to_fit();
}

void PolygonRenderer::drawWorldSurfaces(const std::vector<size_t> &surfaceIdxs) noexcept {
    glEnable(GL_DEPTH_TEST);

    if (r_cull.getValue() != 0) {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CW); 
        glCullFace(r_cull.getValue() == 1 ? GL_BACK : GL_FRONT);
    }

    s_Shader.enable();
    s_Shader.loadMatrices(getOptions());

    for (size_t idx : surfaceIdxs) {
        Surface &surf = m_Surfaces[idx];
        surf.draw();
        m_DrawStats.worldSurfaces++;
    }

    s_Shader.disable();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}
