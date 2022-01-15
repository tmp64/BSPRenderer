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
}

GraphicsStack::~GraphicsStack() {
    AFW_ASSERT_MSG(GPUBuffer::getGlobalMemUsage() == 0, "GPUBuffer leak");
}
