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
        ShaderUniform<glm::mat4> m_TransMat, m_ViewMat, m_ProjMat;
        ShaderUniform<glm::vec3> m_Color;
    };

protected:
    virtual void createSurfaces() override;
    virtual void destroySurfaces() override;
    virtual void drawWorldSurfaces(const std::vector<size_t> &surfaceIdxs) override;

private:
    GLuint m_Vao = 0, m_Vbo = 0;
};

#endif
