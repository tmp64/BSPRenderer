#ifndef RENDERER_SCENE_RENDERER_H
#define RENDERER_SCENE_RENDERER_H
#include <vector>
#include <renderer/surface_renderer.h>
#include <renderer/base_shader.h>
#include <renderer/raii.h>

class SceneRenderer : appfw::utils::NoCopy {
public:
    SceneRenderer();
    ~SceneRenderer();

    /**
     * Returns level set to the renderer.
     */
    inline bsp::Level *getLevel() { return m_pLevel; }

    /**
     * Sets the level.
     * @param   level   Loaded bsp::Level
     * @param   bspPath Path to the .bsp file to load lightmap from (can be empty)
     */
    void setLevel(bsp::Level *level, const fs::path &bspPath);

    /**
     * Sets perspective projection.
     * @param   fov     Horizontal field of view
     * @param   aspect  Aspect ration of the screen (wide / tall)
     * @param   near    Near clipping plane
     * @param   far     Far clipping plane
     */
    void setPerspective(float fov, float aspect, float near, float far);

    /**
     * Sets position and angles of the perspective view.
     * @param   origin  Origin of view
     * @param   angles  Pitch, yaw and roll in degrees
     */
    void setPerspViewOrigin(const glm::vec3 &origin, const glm::vec3 &angles);

    /**
     * Sets size of the viewport.
     */
    void setViewportSize(const glm::ivec2 &size);

    /**
     * Renders the image to the screen.
     */
    void renderScene(GLint targetFb);

    /**
     * Shows ImGui dialog with debug info.
     */
    void showDebugDialog();

private:
    class WorldShader : public BaseShader {
    public:
        WorldShader();
        virtual void create() override;
        void setupUniforms(SceneRenderer &scene);
        void setColor(const glm::vec3 &c);

    private:
        ShaderUniform<glm::mat4> m_ViewMat, m_ProjMat;
        ShaderUniform<glm::vec3> m_Color;
        ShaderUniform<int> m_LightingType;
        ShaderUniform<int> m_TextureType;
        ShaderUniform<int> m_Texture;
        ShaderUniform<int> m_LMTexture;
        ShaderUniform<float> m_TexGamma;
    };

    class SkyBoxShader : public BaseShader {
    public:
        SkyBoxShader();
        virtual void create() override;
        void setupUniforms(SceneRenderer &scene);

    private:
        ShaderUniform<glm::mat4> m_ViewMat, m_ProjMat;
        ShaderUniform<int> m_Texture;
        ShaderUniform<float> m_TexGamma;
        ShaderUniform<float> m_Brightness;
        ShaderUniform<glm::vec3> m_ViewOrigin;
    };

    class PostProcessShader : public BaseShader {
    public:
        PostProcessShader();
        virtual void create() override;
        void setupUniforms();

    private:
        ShaderUniform<int> m_Tonemap;
        ShaderUniform<float> m_Exposure;
        ShaderUniform<float> m_Gamma;
    };

    struct SurfaceVertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texture;
        glm::vec2 lmTexture;
    };

    static_assert(sizeof(SurfaceVertex) == sizeof(float) * 10, "Size of Vertex is invalid");

    struct Surface {
        GLVao m_Vao;
        GLBuffer m_Vbo;
        glm::vec3 m_Color;
        GLsizei m_iVertexCount = 0;
        size_t m_nMatIdx = NULL_MATERIAL;

        // Lightmap info
        std::vector<glm::vec2> m_vLMTexCoords;
        glm::ivec2 m_vLMSize;
        GLTexture m_LMTex;
    };

    struct LevelData {
        SurfaceRenderer::Context viewContext;
        std::vector<Surface> surfaces;
        bool bCustomLMLoaded = false;
        GLTexture skyboxCubemap;
    };

    bsp::Level *m_pLevel = nullptr;
    SurfaceRenderer m_Surf;
    LevelData m_Data;

    // Screen-wide quad
    GLVao m_nQuadVao;
    GLBuffer m_nQuadVbo;
    void createScreenQuad();

    // Framebuffer
    bool m_bNeedRefreshFB = true;
    glm::ivec2 m_vViewportSize = glm::ivec2(1, 1);
    GLuint m_nHdrFramebuffer = 0;
    GLuint m_nColorBuffer = 0;
    GLuint m_nRenderBuffer = 0;
    void recreateFramebuffer();
    void destroyFramebuffer();

    void createSurfaces();
    void loadCustomLightmaps(const fs::path &bspPath);
    void createSurfaceObjects();
    void loadSkyBox();
    std::vector<uint8_t> rotateImage90CW(uint8_t *data, int wide, int tall);
    std::vector<uint8_t> rotateImage90CCW(uint8_t *data, int wide, int tall);

    /**
     * Binds and clears HDB framebuffer
     */
    void prepareHdrFramebuffer();

    /**
     * Sets view constext settings.
     */
    void setupViewContext();

    /**
     * Draws solid BSP faces.
     */
    void drawWorldSurfaces();

     /**
     * Draws BSP faces with SKY texture.
     */
    void drawSkySurfaces();

    /**
     * Post-processes HDB framebuffer (tonemapping, gamma correction) and draws it into active framebuffer.
     */
    void doPostProcessing();

    static inline WorldShader m_sWorldShader;
    static inline SkyBoxShader m_sSkyShader;
    static inline PostProcessShader m_sPostProcessShader;
};

#endif
