#ifndef RENDERER_POLYGON_RENDERER_H
#define RENDERER_POLYGON_RENDERER_H
#include <appfw/utils.h>
#include <renderer/base_shader.h>
#include <renderer/base_renderer.h>

class PolygonRenderer : public BaseRenderer {
public:
    class Shader : public BaseShader {
    public:
        Shader();
        virtual void create() override;
        void loadMatrices(const BaseRenderer::DrawOptions &options);
        void setColor(const glm::vec3 &c);

    private:
        ShaderUniform<glm::mat4> m_ViewMat, m_ProjMat;
        ShaderUniform<glm::vec3> m_Color;
    };

    struct Surface : appfw::utils::NoCopy {
        GLuint m_Vao = 0, m_Vbo = 0;
        glm::vec3 m_Color;
        size_t m_VertexCount = 0;

        Surface(const LevelSurface &baseSurface) noexcept;
        Surface(Surface &&other) noexcept;
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
