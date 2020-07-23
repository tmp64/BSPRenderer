#include <appfw/services.h>
#include <renderer/polygon_renderer.h>

static PolygonRenderer::Shader s_Shader;

//-----------------------------------------------------------------------------
// CShader
//-----------------------------------------------------------------------------
PolygonRenderer::Shader::Shader()
    : BaseShader("BasicWorldShader"), m_TransMat(this, "transMatrix"), m_ViewMat(this, "viewMatrix"),
      m_ProjMat(this, "projMatrix"), m_Color(this, "color") {}

void PolygonRenderer::Shader::create() {
    createProgram();
    createVertexShader("shaders/BasicWorldShader.vert.glsl");
    createFragmentShader("shaders/BasicWorldShader.frag.glsl");
    linkProgram();
}

void PolygonRenderer::Shader::loadMatrices(const BaseRenderer::DrawOptions &options) {
    glm::mat4 proj =
        glm::perspective(glm::radians(options.fov), options.aspect, options.near, options.far);
    m_ProjMat.set(proj);
    //m_ViewMat.set(glm::identity<glm::mat4>());
    m_TransMat.set(glm::identity<glm::mat4>());

    // glm::mat4 m = glm::translate(glm::identity<glm::mat4>(), { 10, 10, 10 });
    // m_ProjMat.Set(m);

    // From quake
    glm::mat4 g_ViewMat = glm::identity<glm::mat4>();
    g_ViewMat = glm::rotate(g_ViewMat, glm::radians(-90.f), {1.0f, 0.0f, 0.0f});
    g_ViewMat = glm::rotate(g_ViewMat, glm::radians(90.f), {0.0f, 0.0f, 1.0f});
    g_ViewMat = glm::rotate(g_ViewMat, glm::radians(-options.viewAngles.z), {1.0f, 0.0f, 0.0f});
    g_ViewMat = glm::rotate(g_ViewMat, glm::radians(-options.viewAngles.x), {0.0f, 1.0f, 0.0f});
    g_ViewMat = glm::rotate(g_ViewMat, glm::radians(-options.viewAngles.y), {0.0f, 0.0f, 1.0f});
    g_ViewMat = glm::translate(g_ViewMat, {-options.viewOrigin.x, -options.viewOrigin.y, -options.viewOrigin.z});
    m_ViewMat.set(g_ViewMat);
}

void PolygonRenderer::Shader::setColor(const glm::vec3 &c) { m_Color.set(c); }

//-----------------------------------------------------------------------------
// PolygonRenderer
//-----------------------------------------------------------------------------
void PolygonRenderer::createSurfaces() {
    glGenVertexArrays(1, &m_Vao);
    glGenBuffers(1, &m_Vbo);
    glBindVertexArray(m_Vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 512, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PolygonRenderer::destroySurfaces() {}

void PolygonRenderer::drawWorldSurfaces(const std::vector<size_t> &surfaceIdxs) {
    for (size_t idx : surfaceIdxs) {
        LevelSurface &surf = m_BaseSurfaces[idx];

        std::vector<glm::vec3> vert;

        for (size_t i = 1; i < surf.vertices.size() - 1; i++) {
            vert.push_back(surf.vertices[0]);
            vert.push_back(surf.vertices[i + 0]);
            vert.push_back(surf.vertices[i + 1]);
        }

        glBindVertexArray(m_Vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * vert.size(), vert.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        /*glDisable(GL_CULL_FACE);
        glCullFace(GL_FRONT);  
        glFrontFace(GL_CCW);*/

        srand(idx);
        auto fnGetRandColor = []() { return (rand() % 256) / 255.f; };
        glm::vec3 randColor = {fnGetRandColor(), fnGetRandColor(), fnGetRandColor()};

        s_Shader.enable();
        s_Shader.loadMatrices(getOptions());
        s_Shader.setColor(randColor);
        glDrawArrays(GL_TRIANGLES, 0, vert.size());
        s_Shader.disable();

        glBindVertexArray(0);

        m_DrawStats.worldSurfaces++;
    }
}
