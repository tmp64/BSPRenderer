#ifndef IMGUI_IMPL_SHADERS_H
#define IMGUI_IMPL_SHADERS_H
#include <material_system/shader.h>

class ImGuiShader : public ShaderT<ImGuiShader> {
public:
    TextureVar m_Tex;
    CustomVar m_ProjMtx;

    ImGuiShader(unsigned type = 0)
        : ShaderT(type) {
        setTitle("ImGuiShader");
        setTypes(SHADER_TYPE_IMGUI | SHADER_TYPE_IMGUI_LINEAR);

        addUniform(m_Tex, "Texture");
        addUniform(m_ProjMtx, "ProjMtx");

        if (type == SHADER_TYPE_IMGUI) {
            setVert("assets:shaders/imgui/gamma.vert");
            setFrag("assets:shaders/imgui/gamma.frag");
        } else if (type == SHADER_TYPE_IMGUI_LINEAR) {
            setVert("assets:shaders/imgui/linear.vert");
            setFrag("assets:shaders/imgui/linear.frag");
            addSharedDef("GAMMA", 2.2f);
            setFramebufferSRGB(true);
        } else if (type != 0) {
            AFW_ASSERT(false);
        }
    }

    void onShaderCompiled() override {
        m_Tex.set(0);
    }

    // Implemented inm imgui_impl_opengl3.cpp
    void onEnabledOnce() override;
};

inline ImGuiShader ImGuiShaderPrototype;

#endif
