#ifndef GRAPHICS_GRAPHICS_STACK_H
#define GRAPHICS_GRAPHICS_STACK_H
#include <glad/glad.h>
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

private:
    int m_iMaxAniso = 1;
};

#endif
