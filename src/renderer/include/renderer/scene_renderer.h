#ifndef RENDERER_SCENE_RENDERER_H
#define RENDERER_SCENE_RENDERER_H
#include <vector>
#include <functional>
#include <app_base/texture_block.h>
#include <renderer/surface_renderer.h>
#include <renderer/base_shader.h>
#include <renderer/raii.h>
#include <renderer/client_entity.h>

class SceneRenderer : appfw::NoCopy {
public:
    /**
     * Called during rendering to update the entity list.
     */
    using EntListCallback = std::function<void()>;

    SceneRenderer();
    ~SceneRenderer();

    struct RenderingStats {
        //! Number of world polygons rendered
        unsigned uRenderedWorldPolys = 0;

        //! Number of sky polygons rendered
        unsigned uRenderedSkyPolys = 0;

        //! Number of brush entity polygons rendered
        unsigned uRenderedBrushEntPolys = 0;

        //! Number of draw calls issued (glDrawArrays, ...)
        unsigned uDrawCallCount = 0;

        //! Time of the last frame
        double flFrameTime = 0;
    };

    /**
     * Returns level set to the renderer.
     */
    inline const bsp::Level *getLevel() { return m_pLevel; }

    /**
     * Begins async level loading.
     * @param   level   Loaded bsp::Level
     * @param   path    Path to the .bsp file to load lightmap from (can be empty)
     * @param   tag     Tag of .bsp
     */
    void beginLoading(const bsp::Level *level, std::string_view path);

    /**
     * Creates an optimized model for a brush model for more effficient solid rendering.
     */
    void optimizeBrushModel(Model *model);

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
     * Sets the entitiy list callback.
     */
    inline void setEntListCallback(const EntListCallback &fn) { m_pfnEntListCb = fn; }

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

    /**
     * During EntListCallback:
     * Adds an entity into rendering list.
     * @returns true if entity was added, false if limit reached
     */
    bool addEntity(ClientEntity *pClent);

private:
    static constexpr int BSP_LIGHTMAP_DIVISOR = 16;
    static constexpr int BSP_LIGHTMAP_BLOCK_SIZE = 1024;
    static constexpr int BSP_LIGHTMAP_PADDING = 2;
    static constexpr uint16_t PRIMITIVE_RESTART_IDX = std::numeric_limits<uint16_t>::max();
    static constexpr int MAX_TRANS_SURFS_PER_MODEL = 512; //!< Maximum number of surfaces per transparent model

    class WorldShader : public BaseShader {
    public:
        WorldShader();
        void setupUniforms(SceneRenderer &scene);
        void setColor(const glm::vec3 &c);

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
        void setupUniforms(SceneRenderer &scene);

    private:
        ShaderUniform<glm::mat4> m_ViewMat, m_ProjMat;
        ShaderUniform<int> m_Texture;
        ShaderUniform<float> m_TexGamma;
        ShaderUniform<float> m_Brightness;
        ShaderUniform<glm::vec3> m_ViewOrigin;
    };

    class BrushEntityShader : public BaseShader {
    public:
        BrushEntityShader();
        void setupSceneUniforms(SceneRenderer &scene);
        void setColor(const glm::vec3 &c);

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

    class PatchesShader : public BaseShader {
    public:
        PatchesShader();
        void setupSceneUniforms(SceneRenderer &scene);
        void setColor(const glm::vec3 &c);

        ShaderUniform<glm::mat4> m_ViewMat, m_ProjMat;
    };

    class LuminanceShader : public BaseShader {
    public:
        LuminanceShader();
    };

    class PostProcessShader : public BaseShader {
    public:
        PostProcessShader();
        void setupUniforms();

    private:
        ShaderUniform<int> m_HdrBuffer;
        ShaderUniform<int> m_LumBuffer;
        ShaderUniform<int> m_Tonemap;
        ShaderUniform<float> m_Gamma;
        ShaderUniform<float> m_WhitePoint;
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
        glm::vec3 m_Color;
        GLsizei m_iVertexCount = 0;
        size_t m_nMatIdx = NULL_MATERIAL;

        uint16_t m_nFirstVertex; //!< Index of first vertex in the vertex buffer

        // BSP lightmap info
        glm::vec2 m_vTextureMins = glm::vec2(0, 0);
        glm::ivec2 m_BSPLMSize = glm::ivec2(0, 0);
        glm::ivec2 m_BSPLMOffset = glm::ivec2(0, 0);

        // Custom lightmap info
        std::vector<glm::vec2> m_vCustomLMTexCoords;
    };

    struct OptBrushModel {
        std::vector<unsigned> surfs; //!< Contains surface indeces sorted by material
    };

    struct LevelData {
        fs::path customLightmapPath;
        SurfaceRenderer::Context viewContext;
        std::vector<Surface> surfaces;
        bool bCustomLMLoaded = false;
        GLTexture skyboxCubemap;
        
        // Lightmaps
        GLTexture bspLightmapBlockTex;
        GLTexture customLightmapBlockTex;

        // Brush geometry rendering
        GLVao surfVao;
        GLBuffer surfVbo;
        GLBuffer surfEbo;
        std::vector<uint16_t> surfEboData;

        // Brush entity rendering
        std::vector<OptBrushModel> optBrushModels;

        // Patches
        uint32_t patchesVerts = 0;
        GLVao patchesVao;
        GLBuffer patchesVbo;
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

        // Lightmaps
        TextureBlock<glm::u8vec3> bspLightmapBlock;
        std::vector<glm::vec3> customLightmapTex;
        glm::ivec2 customLightmapTexSize;

        // Surface objects
        std::future<void> createSurfaceObjectsResult;
        std::vector<std::vector<SurfaceVertex>> surfVertices; //!< Vertices of individual surfaces
        std::vector<SurfaceVertex> allVertices; //!< Vertices of all surfaces
        int maxEboSize = 0; //!< Maximum number of elements in the EBO (if all surfaces are visible at the same time)
    };

    const bsp::Level *m_pLevel = nullptr;
    SurfaceRenderer m_Surf;
    LevelData m_Data;
    RenderingStats m_Stats;
    LoadingStatus m_LoadingStatus;
    std::unique_ptr<LoadingState> m_pLoadingState;
    EntListCallback m_pfnEntListCb;
    std::vector<ClientEntity *> m_SolidEntityList;
    std::vector<ClientEntity *> m_TransEntityList;
    std::vector<size_t> m_SortBuffer;
    unsigned m_uVisibleEntCount = 0;

    // Screen-wide quad
    GLVao m_nQuadVao;
    GLBuffer m_nQuadVbo;
    void createScreenQuad();

    // Framebuffers
    bool m_bNeedRefreshFB = true;
    glm::ivec2 m_vViewportSize = glm::ivec2(1, 1);
    GLFramebuffer m_nHdrFramebuffer;
    GLTexture m_nColorBuffer;
    GLRenderbuffer m_nRenderBuffer;

    GLTexture m_nLumColorBuffer;
    GLFramebuffer m_nLumFramebuffer;

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
    void loadTextures();
    void loadSkyBox();
    void loadPatches();
    void finishLoading();
    std::vector<uint8_t> rotateImage90CW(uint8_t *data, int wide, int tall);
    std::vector<uint8_t> rotateImage90CCW(uint8_t *data, int wide, int tall);

    /**
     * Pre-rendering stuff
     */
    void frameSetup();

    /**
     * Post-rendering stuff.
     * Disables stuff enabled in frameSetup.
     */
    void frameEnd();

    /**
     * Binds and clears HDB framebuffer
     */
    void prepareHdrFramebuffer();

    /**
     * Sets view constext settings.
     */
    void setupViewContext();

    /**
     * Binds lightmap block texture
     */
    void bindLightmapBlock();

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
     * Draws BSP faces with SKY texture using VAOs.
     */
    void drawSkySurfacesVao();

    /**
     * Draws BSP faces with SKY texture using EBO.
     */
    void drawSkySurfacesIndexed();

    /**
     * Draws solid entities
     */
    void drawSolidEntities();

    /**
     * Draws transparent entities
     */
    void drawTransEntities();

    /**
     * Draws a solid brush entity
     */
    void drawSolidBrushEntity(ClientEntity *pClent);

    /**
     * Draws a (maybe transparent) brush entity
     */
    void drawBrushEntity(ClientEntity *pClent);

    /**
     * Draws a brush entity surface.
     */
    void drawBrushEntitySurface(Surface &surf);

    /**
     * Calculates average luminance from mipmap of the HDR framebuffer.
     */
    void calculateAvgLum();

    /**
     * Post-processes HDB framebuffer (tonemapping, gamma correction) and draws it into active framebuffer.
     */
    void doPostProcessing();

    /**
     * Sets up render mode.
     */
    void setRenderMode(RenderMode mode);

    static inline WorldShader m_sWorldShader;
    static inline SkyBoxShader m_sSkyShader;
    static inline BrushEntityShader m_sBrushEntityShader;
    static inline PatchesShader m_sPatchesShader;
    static inline LuminanceShader m_sLuminanceShader;
    static inline PostProcessShader m_sPostProcessShader;
};

#endif
