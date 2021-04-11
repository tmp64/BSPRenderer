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

        //! Number of draw calls issued (glDrawArrays, ...)
        unsigned uDrawCallCount = 0;

        //! Time (us) taken by frame setup
        TimeSmoother uSetupTime;

        //! Time (us) taken by world BSP traversing
        TimeSmoother uWorldBSPTime;

        //! Time (us) taken by world geometry rendering
        TimeSmoother uWorldRenderingTime;

        //! Time (us) taken by sky polygons rendering
        TimeSmoother uSkyRenderingTime;

        //! Time (us) taken by post processing
        TimeSmoother uPostProcessingTime;

        //! Total time (us) taken by scene rendering
        TimeSmoother uTotalRenderingTime;

        inline void clear() {
            uRenderedWorldPolys = 0;
            uRenderedSkyPolys = 0;
            uDrawCallCount = 0;
        }
    };

    /**
     * Returns level set to the renderer.
     */
    inline bsp::Level *getLevel() { return m_pLevel; }

    /**
     * Begins async level loading.
     * @param   level   Loaded bsp::Level
     * @param   path    Path to the .bsp file to load lightmap from (can be empty)
     * @param   tag     Tag of .bsp
     */
    void beginLoading(bsp::Level *level, const std::string &path, const char *tag);

    /**
     * Unloads the level.
     */
    void unloadLevel();

    /**
     * Returns whether the level is loading.
     */
    bool isLoading() { return m_pLoadingState != nullptr; }

    /**
     * Should be called during loading from main thread.
     * @return  Whether loading has finished or not
     */
    bool loadingTick();

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
    static constexpr uint16_t PRIMITIVE_RESTART_IDX = std::numeric_limits<uint16_t>::max();

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

        inline bool operator==(const SurfaceVertex &rhs) {
            return position == rhs.position && normal == rhs.normal && texture == rhs.texture &&
                   bspLMTexture == rhs.bspLMTexture && customLMTexture == rhs.customLMTexture;
        }
    };

    static_assert(sizeof(SurfaceVertex) == sizeof(float) * 12, "Size of Vertex is invalid");

    struct Surface {
        GLVao m_Vao;
        GLBuffer m_Vbo;
        glm::vec3 m_Color;
        GLsizei m_iVertexCount = 0;
        size_t m_nMatIdx = NULL_MATERIAL;

        std::vector<uint16_t> m_VertexIndices;

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
        fs::path customLightmapPath;
        SurfaceRenderer::Context viewContext;
        std::vector<Surface> surfaces;
        bool bCustomLMLoaded = false;
        GLTexture skyboxCubemap;
        
        // BSP lightmaps
        GLTexture bspLightmapBlockTex;

        // World geometry indexed rendering
        GLVao worldVao;
        GLBuffer worldVbo;
        GLBuffer worldEbo;
        std::vector<uint16_t> worldEboData;
    };

    enum class LoadingStatus {
        NotLoading,
        CreateSurfaces, //!< Calls SurfaceRenderer::setLevel and fills m_Data.surfaces
        AsyncTasks, //!< Starts and waits for various tasks
        CreateSurfaceObjects, //!< Creates VAOs and VBOs
    };

    struct LoadingState {
        // CreateSurfaces
        std::future<void> createSurfacesResult;

        // Async tasks
        std::future<void> loadBSPLightmapsResult;
        std::future<void> loadCustomLightmapsResult;
        bool loadBSPLightmapsFinished = false;
        bool loadCustomLightmapsFinished = false;

        // BSP lightmaps
        TextureBlock<glm::u8vec3> bspLightmapBlock;

        // Custom lightmaps
        std::vector<std::vector<glm::vec3>> customLightmaps;

        // Surface objects
        std::future<void> createSurfaceObjectsResult;
        std::vector<std::vector<SurfaceVertex>> surfVertices; //!< Vertices of individual surfaces
        std::vector<SurfaceVertex> allVertices; //!< Vertices of all surfaces
        int maxEboSize = 0; //!< Maximum number of elements in the EBO (if all surfaces are visible at the same time)
    };

    bsp::Level *m_pLevel = nullptr;
    SurfaceRenderer m_Surf;
    LevelData m_Data;
    RenderingStats m_Stats;
    LoadingStatus m_LoadingStatus;
    std::unique_ptr<LoadingState> m_pLoadingState;

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

    void asyncCreateSurfaces();
    void asyncLoadBSPLightmaps();
    void finishLoadBSPLightmaps();
    void asyncLoadCustomLightmaps();
    void finishLoadCustomLightmaps();
    void asyncCreateSurfaceObjects();
    void finishCreateSurfaceObjects();
    void enableSurfaceAttribs();
    void loadSkyBox();
    void finishLoading();
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
     * Draws solid BSP faces.
     */
    void drawWorldSurfacesVao();

    /**
     * Draws solid BSP faces using indexed rendering.
     */
    void drawWorldSurfacesIndexed();

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
