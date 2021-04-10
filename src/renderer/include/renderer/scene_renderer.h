#ifndef RENDERER_SCENE_RENDERER_H
#define RENDERER_SCENE_RENDERER_H
#include <vector>
#include <renderer/surface_renderer.h>
#include <renderer/base_shader.h>
#include <renderer/raii.h>
#include <renderer/time_smoother.h>
#include <renderer/texture_block.h>

class SceneRenderer : appfw::NoCopy {
public:
    SceneRenderer();
    ~SceneRenderer();

    struct RenderingStats {
        //! Number of world polygons rendered
        unsigned uRenderedWorldPolys = 0;

        //! Number of sky polygons rendered
        unsigned uRenderedSkyPolys = 0;

        //! Time (us) taken by world BSP traversing
        TimeSmoother uWorldBSPTime;

        //! Time (us) taken by world geometry rendering
        TimeSmoother uWorldRenderingTime;

        //! Time (us) taken by sky polygons rendering
        TimeSmoother uSkyRenderingTime;

        //! Total time (us) taken by scene rendering
        TimeSmoother uTotalRenderingTime;

        inline void clear() {
            uRenderedWorldPolys = 0;
            uRenderedSkyPolys = 0;
        }
    };

    /**
     * Returns level set to the renderer.
     */
    inline bsp::Level *getLevel() { return m_pLevel; }

    /**
     * Sets the level.
     * @param   level   Loaded bsp::Level
     * @param   path    Path to the .bsp file to load lightmap from (can be empty)
     * @param   tag     Tag of .bsp
     */
    void setLevel(bsp::Level *level, const std::string &path, const char *tag);

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
     * Returns performance stats for last renderScene call.
     */
    inline const RenderingStats &getStats() const { return m_Stats; }

    /**
     * Shows ImGui dialog with debug info.
     */
    void showDebugDialog(const char *title, bool *isVisible = nullptr);

private:
    static constexpr int BSP_LIGHTMAP_DIVISOR = 16;
    static constexpr int BSP_LIGHTMAP_BLOCK_SIZE = 1024;
    static constexpr int BSP_LIGHTMAP_PADDING = 2;

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
        glm::vec2 bspLMTexture;
        glm::vec2 customLMTexture;
    };

    static_assert(sizeof(SurfaceVertex) == sizeof(float) * 12, "Size of Vertex is invalid");

    struct Surface {
        GLVao m_Vao;
        GLBuffer m_Vbo;
        glm::vec3 m_Color;
        GLsizei m_iVertexCount = 0;
        size_t m_nMatIdx = NULL_MATERIAL;

        // BSP lightmap info
        glm::vec2 m_vTextureMins = glm::vec2(0, 0);
        glm::ivec2 m_BSPLMSize = glm::ivec2(0, 0);
        glm::ivec2 m_BSPLMOffset = glm::ivec2(0, 0);

        // Custom lightmap info
        std::vector<glm::vec2> m_vCustomLMTexCoords;
        glm::ivec2 m_vCustomLMSize = glm::ivec2(0, 0);
        GLTexture m_CustomLMTex;
    };

    struct LevelData {
        SurfaceRenderer::Context viewContext;
        std::vector<Surface> surfaces;
        bool bCustomLMLoaded = false;
        GLTexture skyboxCubemap;
        
        // BSP lightmaps
        TextureBlock<glm::u8vec3> bspLightmapBlock;
        GLTexture bspLightmapBlockTex;
    };

    bsp::Level *m_pLevel = nullptr;
    SurfaceRenderer m_Surf;
    LevelData m_Data;
    RenderingStats m_Stats;

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
    void loadBSPLightmaps();
    void loadCustomLightmaps(const std::string &path, const char *tag);
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
