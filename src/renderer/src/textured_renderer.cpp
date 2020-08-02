#include <appfw/services.h>
#include <renderer/textured_renderer.h>

 static_assert(sizeof(TexturedRenderer::Vertex) == sizeof(float) * 8, "Size of Vertex is invalid");

static TexturedRenderer::Shader s_Shader;

//-----------------------------------------------------------------------------
// Shader
//-----------------------------------------------------------------------------
TexturedRenderer::Shader::Shader()
    : BaseShader("TexturedRendererShader")
    , m_ViewMat(this, "uViewMatrix")
    , m_ProjMat(this, "uProjMatrix"),
      m_FullBright(this, "uFullBright") {}

void TexturedRenderer::Shader::create() {
    createProgram();
    createVertexShader("shaders/textured/world.vert");
    createFragmentShader("shaders/textured/world.frag");
    linkProgram();
}

void TexturedRenderer::Shader::loadMatrices(const BaseRenderer::FrameVars &vars) {
    m_ProjMat.set(vars.projMat);
    m_ViewMat.set(vars.viewMat);
    m_FullBright.set(r_fullbright.getValue());
}

//-----------------------------------------------------------------------------
// Surface
//-----------------------------------------------------------------------------
TexturedRenderer::Surface::Surface(const LevelSurface &baseSurface) noexcept {
    // Prepare buffer data
    m_nMatIdx = baseSurface.nMatIndex;
    m_iVertexCount = baseSurface.vertices.size();

    const bsp::BSPTextureInfo &texInfo = *baseSurface.pTexInfo;
    const Material &material = MaterialManager::get().getMaterial(m_nMatIdx);

    std::vector<Vertex> data;
    data.reserve(m_iVertexCount);

    //glm::vec3 normal = glm::normalize(glm::cross(baseSurface.vertices[0], baseSurface.vertices[1]));
    glm::vec3 normal = baseSurface.pPlane->vNormal;
    if (baseSurface.iFlags & SURF_PLANEBACK) {
        normal = -normal;
    }

    for (glm::vec3 pos : baseSurface.vertices) {
        Vertex v;

        // Position
        v.position = pos;

        // Normal
        v.normal = normal;

        // Texture
        v.texture.s = glm::dot(pos, texInfo.vS) + texInfo.fSShift;
        v.texture.s /= material.getWide();

        v.texture.t = glm::dot(pos, texInfo.vT) + texInfo.fTShift;
        v.texture.t /= material.getTall();

        data.push_back(v);
    }

    glGenVertexArrays(1, &m_nVao);
    glGenBuffers(1, &m_nVbo);

    glBindVertexArray(m_nVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_nVbo);

    
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_iVertexCount, data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, position)));
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, texture)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

TexturedRenderer::Surface::Surface(Surface &&from) noexcept {
    if (m_nVao != 0) {
        glDeleteVertexArrays(1, &m_nVao);
    }

    if (m_nVbo != 0) {
        glDeleteBuffers(1, &m_nVbo);
    }

    m_nVao = from.m_nVao;
    m_nVbo = from.m_nVbo;
    m_iVertexCount = from.m_iVertexCount;
    m_nMatIdx = from.m_nMatIdx;

    from.m_nVao = 0;
    from.m_nVbo = 0;
    from.m_iVertexCount = 0;
    from.m_nMatIdx = NULL_MATERIAL;
}

TexturedRenderer::Surface::~Surface() {
    if (m_nVao != 0) {
        glDeleteVertexArrays(1, &m_nVao);
    }

    if (m_nVbo != 0) {
        glDeleteBuffers(1, &m_nVbo);
    }
}

void TexturedRenderer::Surface::draw() noexcept {
    const Material &mat = MaterialManager::get().getMaterial(m_nMatIdx);
    mat.bindTextures();

    glBindVertexArray(m_nVao);

    glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)m_iVertexCount);

    glBindVertexArray(0);
}

//-----------------------------------------------------------------------------
// TexturedRenderer
//-----------------------------------------------------------------------------
void TexturedRenderer::createSurfaces() {
    AFW_ASSERT(m_Surfaces.empty());
    m_Surfaces.reserve(getLevelVars().baseSurfaces.size());

    for (size_t i = 0; i < getLevelVars().baseSurfaces.size(); i++) {
        const LevelSurface &baseSurface = getLevelVars().baseSurfaces[i];
        m_Surfaces.emplace_back(baseSurface);
    }
}

void TexturedRenderer::destroySurfaces() {
    m_Surfaces.clear();
    m_Surfaces.shrink_to_fit();
}

void TexturedRenderer::drawWorldSurfaces(const std::vector<size_t> &surfaceIdxs) noexcept {
    glEnable(GL_DEPTH_TEST);

    if (r_cull.getValue() != 0) {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CW); 
        glCullFace(r_cull.getValue() == 1 ? GL_BACK : GL_FRONT);
    }

    s_Shader.enable();
    s_Shader.loadMatrices(getFrameVars());

    for (size_t idx : surfaceIdxs) {
        Surface &surf = m_Surfaces[idx];
        surf.draw();
        m_DrawStats.worldSurfaces++;
    }

    s_Shader.disable();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}
