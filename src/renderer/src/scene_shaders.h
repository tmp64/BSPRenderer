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
        setVert("assets:shaders/scene/brush.vert");
        setFrag("assets:shaders/scene/brush.frag");

        addUniform(m_uGlobalUniform, "GlobalUniform", GLOBAL_UNIFORM_BIND);
        addUniform(m_uColor, "u_Color");
        addUniform(m_uTexture, "u_Texture");
        addUniform(m_uLMTexture, "u_LMTexture");
        addUniform(m_uTintColor, "u_TintColor");
    }

    void setupUniforms() {
        m_uTexture.set(0);
        m_uLMTexture.set(1);
    }

    void setColor(const glm::vec3 &c) { m_uColor.set(c); }

    void setTintColor(const glm::vec4 &c) { m_uTintColor.set(c); }

private:
    UniformBlock m_uGlobalUniform;
    ShaderUniform<glm::vec3> m_uColor;
    ShaderUniform<int> m_uTexture;
    ShaderUniform<int> m_uLMTexture;
    ShaderUniform<glm::vec4> m_uTintColor;
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

        addUniform(m_uGlobalUniform, "GlobalUniform", GLOBAL_UNIFORM_BIND);
        addUniform(m_Texture, "u_Texture");
    }

    void setupUniforms() {
        m_Texture.set(0);
    }

private:
    UniformBlock m_uGlobalUniform;
    ShaderUniform<int> m_Texture;
};

//----------------------------------------------------------------
// BrushEntityShader
//----------------------------------------------------------------
class SceneRenderer::BrushEntityShader : public BaseShader {
public:
    BrushEntityShader() {
        setTitle("SceneRenderer::BrushEntityShader");
        setVert("assets:shaders/scene/brush.vert");
        setFrag("assets:shaders/scene/brush.frag");

        getDefinitions().addDef("ENTITY_SHADER", 1);

        addUniform(m_uGlobalUniform, "GlobalUniform", GLOBAL_UNIFORM_BIND);
        addUniform(m_uModelMat, "u_ModelMatrix");
        addUniform(m_uColor, "u_Color");
        addUniform(m_uTexture, "u_Texture");
        addUniform(m_uLMTexture, "u_LMTexture");
        addUniform(m_uTintColor, "u_TintColor");
        addUniform(m_uRenderMode, "u_iRenderMode");
        addUniform(m_uFxAmount, "u_flFxAmount");
        addUniform(m_uFxColor, "u_vFxColor");
    }

    void setupUniforms() {
        m_uTexture.set(0);
        m_uLMTexture.set(1);
    }

    void setColor(const glm::vec3 &c) { m_uColor.set(c); }

    void setTintColor(const glm::vec4 &c) { m_uTintColor.set(c); }

    UniformBlock m_uGlobalUniform;
    ShaderUniform<glm::mat4> m_uModelMat;
    ShaderUniform<glm::vec3> m_uColor;
    ShaderUniform<int> m_uTexture;
    ShaderUniform<int> m_uLMTexture;
    ShaderUniform<glm::vec4> m_uTintColor;
    ShaderUniform<int> m_uRenderMode;
    ShaderUniform<float> m_uFxAmount;
    ShaderUniform<glm::vec3> m_uFxColor;
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

        addUniform(m_uGlobalUniform, "GlobalUniform", GLOBAL_UNIFORM_BIND);
        addUniform(m_HdrBuffer, "u_HdrBuffer");
    }

    void setupUniforms() {
        m_HdrBuffer.set(0);
    }

private:
    UniformBlock m_uGlobalUniform;
    ShaderUniform<int> m_HdrBuffer;
};

struct SceneRenderer::Shaders {
    static inline WorldShader world;
    static inline SkyBoxShader skybox;
    static inline BrushEntityShader brushent;
    static inline PatchesShader patches;
    static inline PostProcessShader postprocess;
};

#endif
