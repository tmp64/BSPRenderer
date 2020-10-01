#ifndef RENDERER_POLYGON_RENDERER_H
#define RENDERER_POLYGON_RENDERER_H
#include <appfw/utils.h>
#include <renderer/base_shader.h>
#include <renderer/base_renderer.h>

class TexturedRenderer : public BaseRenderer {
public:
    class Shader : public BaseShader {
    public:
        Shader();
        virtual void create() override;
        void loadMatrices(const BaseRenderer::FrameVars &vars);
        void setColor(const glm::vec3 &c);

    private:
        ShaderUniform<glm::mat4> m_ViewMat, m_ProjMat;
        ShaderUniform<glm::vec3> m_Color;
        ShaderUniform<int> m_FullBright;
        ShaderUniform<int> m_Texture;
        ShaderUniform<int> m_Lightmap;
        ShaderUniform<float> m_TexGamma;
    };

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texture;
        glm::vec2 lmTexture;
    };

    class PostProcessShader : public BaseShader {
    public:
        PostProcessShader();
        virtual void create() override;
        void loadUniforms();

    private:
        ShaderUniform<float> m_Gamma;
    };

    struct Surface : appfw::utils::NoCopy {
        GLuint m_nVao = 0, m_nVbo = 0;
        glm::vec3 m_Color;
        GLuint m_nLightmapTex = 0;
        size_t m_iVertexCount = 0;
        size_t m_nMatIdx = NULL_MATERIAL;

        Surface(const LevelSurface &baseSurface) noexcept;
        Surface(Surface &&from) noexcept;
        ~Surface();

        void draw() noexcept;
    };

    TexturedRenderer();
    virtual ~TexturedRenderer();

    virtual void updateScreenSize(glm::ivec2 size) override;

protected:
    virtual void createSurfaces() override;
    virtual void destroySurfaces() override;
    virtual void drawWorldSurfaces(const std::vector<size_t> &surfaceIdxs) noexcept override;

private:
    std::vector<Surface> m_Surfaces;
    GLuint m_nHdrFramebuffer = 0;
    GLuint m_nColorBuffer = 0;
    GLuint m_nRenderBuffer = 0;
    GLuint m_nQuadVao = 0;
    GLuint m_nQuadVbo = 0;

    void createBuffers();
    void destroyBuffers();

};

#endif
