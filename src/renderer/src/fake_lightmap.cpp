#include "fake_lightmap.h"

SceneRenderer::FakeLightmap::FakeLightmap(SceneRenderer &renderer) {
	// Completely white texture
    constexpr int TEX_SIZE = 4;
    constexpr int COMPONENTS = 1;
    std::vector<uint8_t> data(TEX_SIZE * TEX_SIZE * bsp::NUM_LIGHTSTYLES * COMPONENTS, 255);
    m_Texture.create("SceneRenderer: Fake lightmap");
    m_Texture.initTexture(GraphicsFormat::R8, TEX_SIZE, TEX_SIZE, bsp::NUM_LIGHTSTYLES, false,
                          GL_RED, GL_UNSIGNED_BYTE, data.data());

    // Empty vertex buffer
    std::vector<LightmapVertex> vertexBuffer(renderer.m_uSurfaceVertexBufferSize);
    m_VertBuffer.create(GL_ARRAY_BUFFER, "SceneRenderer: BSP lightmap vertices");
    m_VertBuffer.init(sizeof(LightmapVertex) * vertexBuffer.size(), vertexBuffer.data(),
                      GL_STATIC_DRAW);
}

float SceneRenderer::FakeLightmap::getGamma() {
    return 0.0f;
}

void SceneRenderer::FakeLightmap::bindTexture() {
    m_Texture.bind();
}

void SceneRenderer::FakeLightmap::bindVertBuffer() {
    m_VertBuffer.bind();
}

void SceneRenderer::FakeLightmap::updateFilter(bool) {
    // Do nothing
}
