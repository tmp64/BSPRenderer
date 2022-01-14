#ifndef SCENE_SHADERS_H
#define SCENE_SHADERS_H
#include <material_system/shader.h>
#include <renderer/scene_renderer.h>

extern ConVar<int> r_lighting;
extern ConVar<int> r_texture;
extern ConVar<float> r_texgamma;
extern ConVar<float> r_gamma;

//----------------------------------------------------------------
// WorldShader
//----------------------------------------------------------------
class SceneRenderer::WorldShader : public ShaderT<WorldShader> {
public:
    WorldShader(unsigned type = 0)
        : BaseClass(type) {
        setTitle("SceneRenderer::WorldShader");
        setVert("assets:shaders/scene/brush.vert");
        setFrag("assets:shaders/scene/brush.frag");
        setTypes(SHADER_TYPE_WORLD);

#ifdef RENDERER_SUPPORT_TINTING
        addSharedDef("SUPPORT_TINTING", 1);
#endif

        addUniform(m_uGlobalUniform, "GlobalUniform", GLOBAL_UNIFORM_BIND);
        addUniform(m_uColor, "u_Color");
        addUniform(m_uTexture, "u_Texture");
        addUniform(m_uLMTexture, "u_LMTexture");
    }

    void onShaderCompiled() override {
        m_uTexture.set(0);
        m_uLMTexture.set(TEX_LIGHTMAP);
    }

    void setColor(const glm::vec3 &c) { m_uColor.set(c); }

private:
    UniformBlock m_uGlobalUniform;
    Var<glm::vec3> m_uColor;
    TextureVar m_uTexture;
    TextureVar m_uLMTexture;
};

//----------------------------------------------------------------
// SkyBoxShader
//----------------------------------------------------------------
class SceneRenderer::SkyBoxShader : public ShaderT<SkyBoxShader> {
public:
    SkyBoxShader(unsigned type = 0)
        : BaseClass(type) {
        setTitle("SceneRenderer::WorldShader");
        setVert("assets:shaders/scene/skybox.vert");
        setFrag("assets:shaders/scene/skybox.frag");
        setTypes(SHADER_TYPE_WORLD);

#ifdef RENDERER_SUPPORT_TINTING
        addSharedDef("SUPPORT_TINTING", 1);
#endif

        addUniform(m_uGlobalUniform, "GlobalUniform", GLOBAL_UNIFORM_BIND);
        addUniform(m_Texture, "u_Texture");
    }

    void onShaderCompiled() override {
        m_Texture.set(0);
    }

private:
    UniformBlock m_uGlobalUniform;
    Var<int> m_Texture;
};

//----------------------------------------------------------------
// BrushEntityShader
//----------------------------------------------------------------
class SceneRenderer::BrushEntityShader : public ShaderT<BrushEntityShader> {
public:
    BrushEntityShader(unsigned type = 0)
        : BaseClass(type) {
        setTitle("SceneRenderer::BrushEntityShader");
        setVert("assets:shaders/scene/brush.vert");
        setFrag("assets:shaders/scene/brush.frag");
        setTypes(SHADER_TYPE_BRUSH_MODEL);

#ifdef RENDERER_SUPPORT_TINTING
        addSharedDef("SUPPORT_TINTING", 1);
#endif

        addSharedDef("ENTITY_SHADER", 1);

        addUniform(m_uGlobalUniform, "GlobalUniform", GLOBAL_UNIFORM_BIND);
        addUniform(m_uModelMat, "u_ModelMatrix");
        addUniform(m_uColor, "u_Color");
        addUniform(m_uTexture, "u_Texture");
        addUniform(m_uLMTexture, "u_LMTexture");
        addUniform(m_uRenderMode, "u_iRenderMode");
        addUniform(m_uFxAmount, "u_flFxAmount");
        addUniform(m_uFxColor, "u_vFxColor");
    }

    void onShaderCompiled() override {
        m_uTexture.set(0);
        m_uLMTexture.set(TEX_LIGHTMAP);
    }

    void setColor(const glm::vec3 &c) { m_uColor.set(c); }

    UniformBlock m_uGlobalUniform;
    Var<glm::mat4> m_uModelMat;
    Var<glm::vec3> m_uColor;
    TextureVar m_uTexture;
    TextureVar m_uLMTexture;
    Var<int> m_uRenderMode;
    Var<float> m_uFxAmount;
    Var<glm::vec3> m_uFxColor;
};

//----------------------------------------------------------------
// PatchesShader
//----------------------------------------------------------------
class SceneRenderer::PatchesShader : public ShaderT<PatchesShader> {
public:
    PatchesShader(unsigned type = 0)
        : BaseClass(type) {
        setTitle("SceneRenderer::PatchesShader");
        setVert("assets:shaders/scene/patches.vert");
        setFrag("assets:shaders/scene/patches.frag");
        setTypes(SHADER_TYPE_CUSTOM);

        addUniform(m_ViewMat, "uViewMatrix");
        addUniform(m_ProjMat, "uProjMatrix");
    }

    void setupSceneUniforms(SceneRenderer &scene) {
        m_ProjMat.set(scene.m_Data.viewContext.getProjectionMatrix());
        m_ViewMat.set(scene.m_Data.viewContext.getViewMatrix());
    }

    Var<glm::mat4> m_ViewMat, m_ProjMat;
};

//----------------------------------------------------------------
// PostProcessShader
//----------------------------------------------------------------
class SceneRenderer::PostProcessShader : public ShaderT<PostProcessShader> {
public:
    PostProcessShader(unsigned type = 0)
        : BaseClass(type) {
        setTitle("SceneRenderer::PostProcessShader");
        setVert("assets:shaders/scene/post_processing.vert");
        setFrag("assets:shaders/scene/post_processing.frag");
        setTypes(SHADER_TYPE_BLIT);

        addUniform(m_uGlobalUniform, "GlobalUniform", GLOBAL_UNIFORM_BIND);
        addUniform(m_HdrBuffer, "u_HdrBuffer");
    }

    void onShaderCompiled() override {
        m_HdrBuffer.set(0);
    }

private:
    UniformBlock m_uGlobalUniform;
    TextureVar m_HdrBuffer;
};

struct SceneRenderer::Shaders {
    static inline WorldShader world;
    static inline SkyBoxShader skybox;
    static inline BrushEntityShader brushent;
    static inline PatchesShader patches;
    static inline PostProcessShader postprocess;
};

#endif
