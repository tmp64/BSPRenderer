#ifndef RENDERER_SCENE_SHADERS_H
#define RENDERER_SCENE_SHADERS_H
#include <material_system/shader.h>
#include <renderer/scene_renderer.h>

namespace SceneShaders {

//! Brush Shader
//! Used for all brushes (both world and entities).
class BrushShader : public ShaderT<BrushShader> {
public:
    BrushShader(unsigned type = 0)
        : BaseClass(type) {
        setTitle("SceneShaders::BrushShader");
        setVert("assets:shaders/scene/brush.vert");
        setFrag("assets:shaders/scene/brush.frag");
        setTypes(SHADER_TYPE_WORLD | SHADER_TYPE_BRUSH_MODEL);

        if (type == SHADER_TYPE_BRUSH_MODEL) {
            addSharedDef("ENTITY_SHADER", 1);

            addUniform(m_uModelMat, "u_ModelMatrix");
            addUniform(m_uRenderMode, "u_iRenderMode");
            addUniform(m_uFxAmount, "u_flFxAmount");
            addUniform(m_uFxColor, "u_vFxColor");
        }

#ifdef RENDERER_SUPPORT_TINTING
        addSharedDef("SUPPORT_TINTING", 1);
#endif
        addUniform(m_uGlobalUniform, "GlobalUniform", SceneRenderer::GLOBAL_UNIFORM_BIND);
        addUniform(m_uTexture, "u_Texture");
        addUniform(m_uLMTexture, "u_LMTexture");
    }

    void onShaderCompiled() override {
        m_uTexture.set(0);
        m_uLMTexture.set(SceneRenderer::TEX_LIGHTMAP);
    }

    void setModelMatrix(glm::mat4 mat) { m_uModelMat.set(mat); }
    void setRenderMode(int renderMode) { m_uRenderMode.set(renderMode); }
    void setRenderFx(float amount, glm::vec3 color) {
        m_uFxAmount.set(amount);
        m_uFxColor.set(color);
    }

private:
    UniformBlock m_uGlobalUniform;
    Var<glm::mat4> m_uModelMat;
    TextureVar m_uTexture;
    TextureVar m_uLMTexture;
    Var<int> m_uRenderMode;
    Var<float> m_uFxAmount;
    Var<glm::vec3> m_uFxColor;
};

//! Skybox shader.
//! Used for the skybox. Entities cannot be the skybox.
class SkyboxShader : public BrushShader {
public:
    SkyboxShader(unsigned type = 0)
        : BrushShader(type) {
        setTitle("SceneShaders::SkyShader");
        setVert("assets:shaders/scene/skybox.vert");
        setFrag("assets:shaders/scene/skybox.frag");
        setTypes(SHADER_TYPE_WORLD);
    }

private:
    std::unique_ptr<Shader> createShaderInfoInstance(unsigned typeIdx) override {
        return std::make_unique<SkyboxShader>(typeIdx);
    }
};

//! Patches shader.
//! Used to draw the patches.
class PatchesShader : public ShaderT<PatchesShader> {
public:
    PatchesShader(unsigned type = 0)
        : BaseClass(type) {
        setTitle("SceneShaders::PatchesShader");
        setVert("assets:shaders/scene/patches.vert");
        setFrag("assets:shaders/scene/patches.frag");
        setTypes(SHADER_TYPE_CUSTOM);

        addUniform(m_uGlobalUniform, "GlobalUniform", SceneRenderer::GLOBAL_UNIFORM_BIND);
    }

private:
    UniformBlock m_uGlobalUniform;
};

//! Postprocessing shader.
class PostProcessShader : public ShaderT<PostProcessShader> {
public:
    PostProcessShader(unsigned type = 0)
        : BaseClass(type) {
        setTitle("SceneShaders::PostProcessShader");
        setVert("assets:shaders/scene/post_processing.vert");
        setFrag("assets:shaders/scene/post_processing.frag");
        setTypes(SHADER_TYPE_BLIT);

        addUniform(m_uGlobalUniform, "GlobalUniform", SceneRenderer::GLOBAL_UNIFORM_BIND);
        addUniform(m_HdrBuffer, "u_HdrBuffer");
    }

    void onShaderCompiled() override { m_HdrBuffer.set(0); }

private:
    UniformBlock m_uGlobalUniform;
    TextureVar m_HdrBuffer;
};

struct Shaders {
    static inline BrushShader brush;
    static inline SkyboxShader skybox;
    static inline PatchesShader patches;
    static inline PostProcessShader postprocess;
};

} // namespace SceneShaders

#endif