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
    };

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texture;
        glm::vec2 lmTexture;
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

protected:
    virtual void createSurfaces() override;
    virtual void destroySurfaces() override;
    virtual void drawWorldSurfaces(const std::vector<size_t> &surfaceIdxs) noexcept override;

private:
    std::vector<Surface> m_Surfaces;

};

#endif
