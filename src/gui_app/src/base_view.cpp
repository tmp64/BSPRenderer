#include <stdexcept>
#include <set>
#include <imgui.h>
#include <appfw/services.h>
#include <renderer/base_shader.h>
#include <renderer/utils.h>
#include <gui_app/base_view.h>

#ifdef GLAD_OPENGL_ES
#define glTexImage2DMultisample glTexStorage2DMultisample
#endif

namespace {

int g_iMaxMSAALevel = 0;

}

static ConVar<int> v_msaa_level("v_msaa_level", 0, "Level of MSAA (1-4)",
[](const int &, const int &newVal) {
    if (appfw::platform::isAndroid()) {
        logError("MSAA is not supported on Android.");
        return false;
    }

    if (newVal < 0 || newVal > g_iMaxMSAALevel) {
        logError("Invalid MSAA level: 0-{} allowed.", g_iMaxMSAALevel);
        return false;
    } else {
        return true;
    }
});

//-----------------------------------------------------------
// GridShader
//-----------------------------------------------------------
class BaseView::GridShader : public BaseShader {
public:
    GridShader()
        : BaseShader("ViewGridShader")
        , m_ViewMat(this, "uViewMatrix")
        , m_ProjMat(this, "uProjMatrix")
        , m_Color(this, "uColor") {}

    virtual void create() override {
        createProgram();
        createVertexShader(getFileSystem().findFile("shaders/base_view/grid.vert", "assets").u8string().c_str());
        createFragmentShader(getFileSystem().findFile("shaders/base_view/grid.frag", "assets").u8string().c_str());
        linkProgram();
    }

    void loadMatrices(const BaseView &view) {
        m_ProjMat.set(view.getProjMat());
        m_ViewMat.set(view.getViewMat());
    }

    void setColor(const glm::vec3 &c) { m_Color.set(c); }

    static BaseView::GridShader instance;

private:
    ShaderUniform<glm::mat4> m_ViewMat, m_ProjMat;
    ShaderUniform<glm::vec3> m_Color;
};

BaseView::GridShader BaseView::GridShader::instance;

//-----------------------------------------------------------
// BaseView
//-----------------------------------------------------------
BaseView::BaseView() {
    updateViewportSize({1, 1});

    if (!appfw::platform::isAndroid()) {
        glGetIntegerv(GL_MAX_SAMPLES, &g_iMaxMSAALevel);
        v_msaa_level.setValue(g_iMaxMSAALevel);
        logInfo("Maximum MSAA level: {}", g_iMaxMSAALevel);
    } else {
        g_iMaxMSAALevel = 0;
    }

    // Create line VBO and VAO
    glGenVertexArrays(1, &m_nLineVao);
    glGenBuffers(1, &m_nLineVbo);

    glBindVertexArray(m_nLineVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_nLineVbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 2, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

BaseView::~BaseView() { destroyBuffers(); }

void BaseView::tick(const char *title, bool *isOpen) {
    // Update MSAA level
    if (m_iMSAALevel != v_msaa_level.getValue()) {
        m_iMSAALevel = v_msaa_level.getValue();
        recreateBuffers();
    }

    // Show view
    ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
    if ((isOpen && !*isOpen) || !ImGui::Begin(title, isOpen)) {
        ImGui::End();
        return;
    }

    ImVec2 vMin = ImGui::GetWindowContentRegionMin();
    ImVec2 vMax = ImGui::GetWindowContentRegionMax();
    int winWide = (int)(vMax.x - vMin.x);
    int winTall = (int)(vMax.y - vMin.y);

    if (winTall <= 0) {
        ImGui::End();
        return;
    }

    ImVec2 imageSize;

    if (m_bAutoSize) {
        updateViewportSize({winWide, winTall});
        imageSize = {(float)winWide, (float)winTall};
    } else {
        // Scale, keep proportions
        float wideRatio = (float)winWide / m_vViewportSize.x;
        float tallRatio = (float)winTall / m_vViewportSize.y;

        if (wideRatio <= tallRatio) {
            imageSize.x = (float)winWide;
            imageSize.y = (float)(int)(winWide * (float)m_vViewportSize.y / m_vViewportSize.x);
        } else {
            imageSize.x = (float)(int)(winTall * (float)m_vViewportSize.x / m_vViewportSize.y);
            imageSize.y = (float)winTall;
        }
        ImGui::SetCursorPos(
            {std::max((winWide - imageSize.x) / 2 + vMin.x, 0.f), std::max((winTall - imageSize.y) / 2 + vMin.y, 0.f)});
    }

    renderScene();

    ImVec2 uv0 = {0, 1};
    ImVec2 uv1 = {1, 0};
    ImGui::Image((void *)(intptr_t)m_nColorBuffer, imageSize, uv0, uv1);

    ImGui::End();
}

void BaseView::setCameraPos(glm::vec3 pos) { m_vPos = pos; }

void BaseView::setCameraRot(glm::vec3 rot) { m_vRot = rot; }

void BaseView::setForcedSize(glm::ivec2 size) {
    m_bAutoSize = false;
    updateViewportSize(size);
}

void BaseView::updateViewportSize(glm::ivec2 size) {
    if (m_vViewportSize == size) {
        return;
    }

    m_vViewportSize = size;
    recreateBuffers();
}

void BaseView::recreateBuffers() {
    destroyBuffers();

    if (m_iMSAALevel > 0) {
        // Create MSAA framebuffer
        glGenFramebuffers(1, &m_nMSAAFramebuffer);

        // Create color buffer
        glGenTextures(1, &m_nMSAAColorBuffer);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_nMSAAColorBuffer);

        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_iMSAALevel, GL_RGB, m_vViewportSize.x, m_vViewportSize.y,
                                true);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

        // Create depth buffer (renderbuffer)
        glGenRenderbuffers(1, &m_nMSAARenderBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_nMSAARenderBuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_iMSAALevel, GL_DEPTH24_STENCIL8, m_vViewportSize.x,
                                         m_vViewportSize.y);

        // Attach buffers
        glBindFramebuffer(GL_FRAMEBUFFER, m_nMSAAFramebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_nMSAAColorBuffer, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_nMSAARenderBuffer);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("MSAA framebuffer not complete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Create normal framebuffer
        glGenFramebuffers(1, &m_nFramebuffer);

        // create a color attachment texture
        glGenTextures(1, &m_nColorBuffer);
        glBindTexture(GL_TEXTURE_2D, m_nColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_vViewportSize.x, m_vViewportSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Attach buffers
        glBindFramebuffer(GL_FRAMEBUFFER, m_nFramebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_nColorBuffer, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("Framebuffer not complete");

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        glGenFramebuffers(1, &m_nFramebuffer);

        // Create color buffer
        glGenTextures(1, &m_nColorBuffer);
        glBindTexture(GL_TEXTURE_2D, m_nColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_vViewportSize.x, m_vViewportSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE,
                     NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Create depth buffer (renderbuffer)
        glGenRenderbuffers(1, &m_nRenderBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_nRenderBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_vViewportSize.x, m_vViewportSize.y);

        // Attach buffers
        glBindFramebuffer(GL_FRAMEBUFFER, m_nFramebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_nColorBuffer, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_nRenderBuffer);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("Framebuffer not complete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void BaseView::destroyBuffers() {
    if (m_nFramebuffer) {
        glDeleteFramebuffers(1, &m_nFramebuffer);
        m_nFramebuffer = 0;
    }

    if (m_nColorBuffer) {
        glDeleteTextures(1, &m_nColorBuffer);
        m_nColorBuffer = 0;
    }

    if (m_nRenderBuffer) {
        glDeleteRenderbuffers(1, &m_nRenderBuffer);
        m_nRenderBuffer = 0;
    }

    if (m_nMSAAFramebuffer) {
        glDeleteFramebuffers(1, &m_nMSAAFramebuffer);
        m_nMSAAFramebuffer = 0;
    }

    if (m_nMSAAColorBuffer) {
        glDeleteTextures(1, &m_nMSAAColorBuffer);
        m_nMSAAColorBuffer = 0;
    }

    if (m_nMSAARenderBuffer) {
        glDeleteRenderbuffers(1, &m_nMSAARenderBuffer);
        m_nMSAARenderBuffer = 0;
    }
}

void BaseView::renderScene() {
    // Set up framebuffer
    if (m_iMSAALevel > 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_nMSAAFramebuffer);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, m_nFramebuffer);
    }

    glViewport(0, 0, m_vViewportSize.x, m_vViewportSize.y);
    glClearColor(0x40 / 255.0f, 0x40 / 255.0f, 0x40 / 255.0f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLineWidth(1);

    // Draw ground and axis
    if (m_bDrawGroundGrid) {
        glEnable(GL_DEPTH_TEST);
        drawGroundGrid();
        glDisable(GL_DEPTH_TEST);
    }

    if (m_bDrawAxis) {
        drawAxis();
    }

    //glEnable(GL_DEPTH_TEST);

    draw();

    // draw may have changed it
    glLineWidth(1);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Blit MSAA buffer into normal buffer
    if (m_iMSAALevel > 0) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_nMSAAFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_nFramebuffer);
        glBlitFramebuffer(0, 0, m_vViewportSize.x, m_vViewportSize.y, 0, 0, m_vViewportSize.x, m_vViewportSize.y,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
}

void BaseView::drawAxis() {
    constexpr float LEN = 0.1f;
    glm::vec3 org = {0.f, 0.f, 0.f};

    drawLine(org, {LEN, 0.f, 0.f}, {1.f, 0.f, 0.f});
    drawLine(org, {0.f, LEN, 0.f}, {0.f, 1.f, 0.f});
    drawLine(org, {0.f, 0.f, LEN}, {0.f, 0.f, 1.f});
}

void BaseView::drawGroundGrid() {
    constexpr int LINE_COUNT = 10;
    constexpr float GRID_SIZE = 0.1f;

    for (int i = 0; i <= LINE_COUNT; i++) {
        float pos = -GRID_SIZE * LINE_COUNT / 2 + GRID_SIZE * i;
        glm::vec3 col = glm::vec3{1.f, 1.f, 1.f} * 0.7f;
        drawLine({-GRID_SIZE * LINE_COUNT / 2, pos, 0}, {GRID_SIZE * LINE_COUNT / 2, pos, 0}, col);
        drawLine({pos, -GRID_SIZE * LINE_COUNT / 2, 0}, {pos, GRID_SIZE * LINE_COUNT / 2, 0}, col);

    }
}

void BaseView::drawLine(glm::vec3 a, glm::vec3 b, glm::vec3 color) {
    GridShader::instance.enable();
    GridShader::instance.loadMatrices(*this);
    GridShader::instance.setColor(color);

    glm::vec3 data[2] = {a, b};
    
    glBindVertexArray(m_nLineVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_nLineVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);

    GridShader::instance.disable();
}
