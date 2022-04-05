#include <graphics/graphics_stack.h>
#include <graphics/gpu_buffer.h>

ConVar<bool> gpu_ui("gpu_ui", false, "");

GraphicsStack::GraphicsStack() {
    if (isAnisoSupported()) {
        float maxAniso = 1;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        m_iMaxAniso = (int)maxAniso;
    } else {
        printw("Anisotropic filtering is not supported by the GPU.");
        printw("Trilinear filtering will cause textures to appear blurry.");
        m_iMaxAniso = 1;
    }

    createBlitQuad();
}

GraphicsStack::~GraphicsStack() {
    m_BlitQuadVbo.destroy();
    AFW_ASSERT_MSG(GPUBuffer::getGlobalMemUsage() == 0, "GPUBuffer leak");
}

void GraphicsStack::blit() {
    glBindVertexArray(m_BlitQuadVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void GraphicsStack::createBlitQuad() {
    // clang-format off
    const float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // clang-format on

    m_BlitQuadVao.create();
    m_BlitQuadVbo.create(GL_ARRAY_BUFFER, "Blit Quad VBO");

    glBindVertexArray(m_BlitQuadVao);
    m_BlitQuadVbo.bind();
    m_BlitQuadVbo.init(sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glBindVertexArray(0);
}
