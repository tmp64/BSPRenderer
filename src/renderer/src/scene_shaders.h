#ifndef SCENE_SHADERS_H
#define SCENE_SHADERS_H
#include <renderer/scene_renderer.h>

extern ConVar<int> r_lighting;
extern ConVar<int> r_texture;
extern ConVar<float> r_texgamma;
extern ConVar<float> r_gamma;

//----------------------------------------------------------------
// WorldShader
//----------------------------------------------------------------
class SceneRenderer::WorldShader : public BaseShader {
public:
    WorldShader() {
        setTitle("SceneRenderer::WorldShader");
        setVert("assets:shaders/scene/world.vert");
        setFrag("assets:shaders/scene/world.frag");

        addUniform(m_ViewMat, "uViewMatrix");
        addUniform(m_ProjMat, "uProjMatrix");
        addUniform(m_Color, "uColor");
        addUniform(m_LightingType, "uLightingType");
        addUniform(m_TextureType, "uTextureType");
        addUniform(m_Texture, "uTexture");
        addUniform(m_LMTexture, "uLMTexture");
        addUniform(m_TexGamma, "uTexGamma");
    }

    void setupUniforms(SceneRenderer &scene) {
        m_ProjMat.set(scene.m_Data.viewContext.getProjectionMatrix());
        m_ViewMat.set(scene.m_Data.viewContext.getViewMatrix());
        m_LightingType.set(r_lighting.getValue());
        m_TextureType.set(r_texture.getValue());
        m_Texture.set(0);
        m_LMTexture.set(1);
        m_TexGamma.set(r_texgamma.getValue());
    }

    void setColor(const glm::vec3 &c) { m_Color.set(c); }

    ShaderUniform<glm::mat4> m_ViewMat, m_ProjMat;
    ShaderUniform<glm::vec3> m_Color;
    ShaderUniform<int> m_LightingType;
    ShaderUniform<int> m_TextureType;
    ShaderUniform<int> m_Texture;
    ShaderUniform<int> m_LMTexture;
    ShaderUniform<float> m_TexGamma;

private:
    
};

//----------------------------------------------------------------
// SkyBoxShader
//----------------------------------------------------------------
class SceneRenderer::SkyBoxShader : public BaseShader {
public:
    SkyBoxShader() {
        setTitle("SceneRenderer::WorldShader");
        setVert("assets:shaders/scene/skybox.vert");
        setFrag("assets:shaders/scene/skybox.frag");

        addUniform(m_ViewMat, "uViewMatrix");
        addUniform(m_ProjMat, "uProjMatrix");
        addUniform(m_Texture, "uTexture");
        addUniform(m_TexGamma, "uTexGamma");
        addUniform(m_ViewOrigin, "uViewOrigin");
    }

    void setupUniforms(SceneRenderer &scene) {
        m_ProjMat.set(scene.m_Data.viewContext.getProjectionMatrix());
        m_ViewMat.set(scene.m_Data.viewContext.getViewMatrix());
        m_Texture.set(0);
        m_TexGamma.set(r_texgamma.getValue());
        m_ViewOrigin.set(scene.m_Data.viewContext.getViewOrigin());
    }

private:
    ShaderUniform<glm::mat4> m_ViewMat, m_ProjMat;
    ShaderUniform<int> m_Texture;
    ShaderUniform<float> m_TexGamma;
    ShaderUniform<glm::vec3> m_ViewOrigin;
};

//----------------------------------------------------------------
// BrushEntityShader
//----------------------------------------------------------------
class SceneRenderer::BrushEntityShader : public BaseShader {
public:
    BrushEntityShader() {
        setTitle("SceneRenderer::BrushEntityShader");
        setVert("assets:shaders/scene/brush_entity.vert");
        setFrag("assets:shaders/scene/brush_entity.frag");

        addUniform(m_ModelMat, "uModelMatrix");
        addUniform(m_ViewMat, "uViewMatrix");
        addUniform(m_ProjMat, "uProjMatrix");
        addUniform(m_Color, "uColor");
        addUniform(m_LightingType, "uLightingType");
        addUniform(m_TextureType, "uTextureType");
        addUniform(m_Texture, "uTexture");
        addUniform(m_LMTexture, "uLMTexture");
        addUniform(m_TexGamma, "uTexGamma");
        addUniform(m_RenderMode, "uRenderMode");
        addUniform(m_FxAmount, "uFxAmount");
        addUniform(m_FxColor, "uFxColor");
    }

    void setupSceneUniforms(SceneRenderer &scene) {
        m_ProjMat.set(scene.m_Data.viewContext.getProjectionMatrix());
        m_ViewMat.set(scene.m_Data.viewContext.getViewMatrix());
        m_LightingType.set(r_lighting.getValue());
        m_TextureType.set(r_texture.getValue());
        m_Texture.set(0);
        m_LMTexture.set(1);
        m_TexGamma.set(r_texgamma.getValue());
    }

    void setColor(const glm::vec3 &c) { m_Color.set(c); }

    ShaderUniform<glm::mat4> m_ModelMat, m_ViewMat, m_ProjMat;
    ShaderUniform<glm::vec3> m_Color;
    ShaderUniform<int> m_LightingType;
    ShaderUniform<int> m_TextureType;
    ShaderUniform<int> m_Texture;
    ShaderUniform<int> m_LMTexture;
    ShaderUniform<float> m_TexGamma;
    ShaderUniform<int> m_RenderMode;
    ShaderUniform<float> m_FxAmount;
    ShaderUniform<glm::vec3> m_FxColor;
};

//----------------------------------------------------------------
// PatchesShader
//----------------------------------------------------------------
class SceneRenderer::PatchesShader : public BaseShader {
public:
    PatchesShader() {
        setTitle("SceneRenderer::PatchesShader");
        setVert("assets:shaders/scene/patches.vert");
        setFrag("assets:shaders/scene/patches.frag");

        addUniform(m_ViewMat, "uViewMatrix");
        addUniform(m_ProjMat, "uProjMatrix");
    }

    void setupSceneUniforms(SceneRenderer &scene) {
        m_ProjMat.set(scene.m_Data.viewContext.getProjectionMatrix());
        m_ViewMat.set(scene.m_Data.viewContext.getViewMatrix());
    }

    ShaderUniform<glm::mat4> m_ViewMat, m_ProjMat;
};

//----------------------------------------------------------------
// PostProcessShader
//----------------------------------------------------------------
class SceneRenderer::PostProcessShader : public BaseShader {
public:
    PostProcessShader() {
        setTitle("SceneRenderer::PostProcessShader");
        setVert("assets:shaders/scene/post_processing.vert");
        setFrag("assets:shaders/scene/post_processing.frag");

        addUniform(m_HdrBuffer, "uHdrBuffer");
        addUniform(m_Gamma, "uGamma");
    }

    void setupUniforms() {
        m_HdrBuffer.set(0);
        m_Gamma.set(r_gamma.getValue());
    }

private:
    ShaderUniform<int> m_HdrBuffer;
    ShaderUniform<float> m_Gamma;
};

struct SceneRenderer::Shaders {
    static inline WorldShader world;
    static inline SkyBoxShader skybox;
    static inline BrushEntityShader brushent;
    static inline PatchesShader patches;
    static inline PostProcessShader postprocess;
};

#endif
