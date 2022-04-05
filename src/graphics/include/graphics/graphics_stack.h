#ifndef GRAPHICS_GRAPHICS_STACK_H
#define GRAPHICS_GRAPHICS_STACK_H
#include <glad/glad.h>
#include <graphics/gpu_buffer.h>
#include <app_base/app_component.h>

class GraphicsStack : public AppComponentBase<GraphicsStack> {
public:
    GraphicsStack();
    ~GraphicsStack();

    //! @returns whether anisotropic filtering is supported by the hardware.
    inline bool isAnisoSupported() {
        return GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic;
    }

    //! @returns maximum supported anisotropy level.
    inline int getMaxAniso() { return m_iMaxAniso; }

    //! Issues a blit draw call: draws a screen-wide quad with currently enabled shader.
    //! Resets VAO to 0. See SHADER_TYPE_BLIT for vertex attribs.
    void blit();

private:
    int m_iMaxAniso = 1;

    GLVao m_BlitQuadVao;
    GPUBuffer m_BlitQuadVbo;

    void createBlitQuad();
};

#endif
