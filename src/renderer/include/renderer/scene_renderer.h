#ifndef RENDERER_SCENE_RENDERER_H
#define RENDERER_SCENE_RENDERER_H
#include <vector>
#include <functional>
#include <app_base/texture_block.h>
#include <graphics/raii.h>
#include <graphics/texture2d.h>
#include <graphics/texture2d_array.h>
#include <graphics/texture_cube.h>
#include <graphics/gpu_buffer.h>
#include <graphics/render_buffer.h>
#include <material_system/material_system.h>
#include <renderer/surface_renderer.h>
#include <renderer/client_entity.h>

class IRendererEngine;

class SceneRenderer : appfw::NoCopy {
public:
    struct Shaders;

    static constexpr int GLOBAL_UNIFORM_BIND = 0;

    enum TextureBinds
    {
        TEX_LIGHTMAP = Material::MAX_TEXTURES,
        TEX_LIGHTSTYLES,
    };

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

    //! Sets the engine.
    inline void setEngine(IRendererEngine *engine) { m_pEngine = engine; }

    //! Returns level set to the renderer.
    inline const bsp::Level *getLevel() { return m_pLevel; }

    //! Begins async level loading.
    //! @param   level   Loaded bsp::Level
    //! @param   path    Path to the .bsp file to load lightmap from (can be empty)
    //! @param   tag     Tag of .bsp
    void beginLoading(const bsp::Level *level, std::string_view path);

    //! Creates an optimized model for a brush model for more effficient solid rendering.
    void optimizeBrushModel(Model *model);

    //! Unloads the level.
    void unloadLevel();

    //! Returns whether the level is loading.
    bool isLoading() { return m_pLoadingState != nullptr; }

    //! Should be called during loading from main thread.
    //! @return  Whether loading has finished or not
    bool loadingTick();

    //! Sets perspective projection.
    //! @param   fov     Horizontal field of view
    //! @param   aspect  Aspect ration of the screen (wide / tall)
    //! @param   near    Near clipping plane
    //! @param   far     Far clipping plane
    void setPerspective(float fov, float aspect, float near, float far);

    //! Sets position and angles of the perspective view.
    //! @param   origin  Origin of view
    //! @param   angles  Pitch, yaw and roll in degrees
    void setPerspViewOrigin(const glm::vec3 &origin, const glm::vec3 &angles);

    //! Sets size of the viewport.
    void setViewportSize(const glm::ivec2 &size);

    //! Renders the image to the screen.
    void renderScene(GLint targetFb, float flSimTime, float flTimeDelta);

    //! Returns performance stats for last renderScene call.
    inline const RenderingStats &getStats() const { return m_Stats; }

    //! Shows ImGui dialog with debug info.
    void showDebugDialog(const char *title, bool *isVisible = nullptr);

    //! Clears the entity list.
    void clearEntities();

    //! Adds an entity into rendering list.
    //! @returns true if entity was added, false if limit reached
    bool addEntity(ClientEntity *pClent);

    //! @returns the material used by the surface.
    Material *getSurfaceMaterial(int surface);

    //! Reloads the custom lightmap file synchronously.
    //! Can't be called during loading.
    void reloadCustomLightmaps();

    //! Sets linear intensity scale of a lightstyle.
    inline void setLightstyleScale(int lightstyle, float scale) {
        m_Data.flLightstyleScale[lightstyle] = scale;
    }

#ifdef RENDERER_SUPPORT_TINTING
    //! Allows tinting world or entity surfaces.
    //! @param  surface The surface index
    //! @param  color   Color in gamma space
    void setSurfaceTint(int surface, glm::vec4 color);
#endif

private:
    static constexpr int BSP_LIGHTMAP_DIVISOR = 16;
    static constexpr int MAX_BSP_LIGHTMAP_BLOCK_SIZE = 2048;
    static constexpr float BSP_LIGHTMAP_BLOCK_WASTED = 0.40f; //!< How much area is assumed to be wasted due to packing
    static constexpr int BSP_LIGHTMAP_PADDING = 2;
    static constexpr uint16_t PRIMITIVE_RESTART_IDX = std::numeric_limits<uint16_t>::max();
    static constexpr int MAX_TRANS_SURFS_PER_MODEL = 512; //!< Maximum number of surfaces per transparent model

    enum class LoadingStatus
    {
        NotLoading,
        CreateSurfaces,       //!< Calls SurfaceRenderer::setLevel and fills m_Data.surfaces
        AsyncTasks,           //!< Starts and waits for various tasks
        CreateSurfaceObjects, //!< Creates VAOs and VBOs
    };

    enum class LightmapType
    {
        BSP = 0,
        Custom = 1,
    };

    struct SurfaceVertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texture;
        glm::ivec4 lightstyle;
    };

    struct Surface {
        glm::vec3 m_Color;
        GLsizei m_iVertexCount = 0;
        Material *m_pMat = nullptr;

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

    struct GlobalUniform {
        glm::mat4 mMainProj;
        glm::mat4 mMainView;
        glm::vec4 vMainViewOrigin; // xyz
        glm::vec4 vflParams1;      // x tex gamma, y screen gamma, z sim time, w sim time delta
        glm::vec4 vflParams2;      // x lightmap gamma
        glm::ivec4 viParams1;      // x is texture type, y is lighting type
    };

    struct LevelData {
        fs::path customLightmapPath;
        SurfaceRenderer::Context viewContext;
        std::vector<Surface> surfaces;
        bool bCustomLMLoaded = false;
        
        // Lightmaps
        Texture2DArray bspLightmapBlockTex;
        Texture2DArray customLightmapBlockTex;
        GPUBuffer bspLightmapCoords;
        GPUBuffer customLightmapCoords;
        LightmapType lightmapType = LightmapType::Custom;

        // Surface lighting
        float flLightstyleScale[MAX_LIGHTSTYLES] = {};

        // Brush geometry rendering
        GLVao surfVao;
        GPUBuffer surfVbo;
        GPUBuffer surfEbo;
        std::vector<uint16_t> surfEboData;

        // Brush entity rendering
        std::vector<OptBrushModel> optBrushModels;

#ifdef RENDERER_SUPPORT_TINTING
        // Tinting
        GPUBuffer surfTintBuf;
#endif

        // Patches
        uint32_t patchesVerts = 0;
        GLVao patchesVao;
        GPUBuffer patchesVbo;
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
        std::vector<glm::u8vec3> bspLightmapBlock;
        int iBspLightmapSize = 0;
        std::vector<glm::vec3> customLightmapTex;
        std::vector<glm::vec3> patchBuffer;
        glm::ivec2 customLightmapTexSize;
        std::vector<glm::vec2> bspLMCoordsBuf;
        std::vector<glm::vec2> customLMCoordsBuf;

        // Surface objects
        std::future<void> createSurfaceObjectsResult;
        std::vector<std::vector<SurfaceVertex>> surfVertices; //!< Vertices of individual surfaces
        std::vector<SurfaceVertex> allVertices; //!< Vertices of all surfaces
        int maxEboSize = 0; //!< Maximum number of elements in the EBO (if all surfaces are visible at the same time)
    };

    IRendererEngine *m_pEngine = nullptr;
    const bsp::Level *m_pLevel = nullptr;
    SurfaceRenderer m_Surf;
    LevelData m_Data;
    RenderingStats m_Stats;
    LoadingStatus m_LoadingStatus;
    std::unique_ptr<LoadingState> m_pLoadingState;
    Material *m_pSkyboxMaterial = nullptr;
    Material *m_pPatchesMaterial = nullptr;
    Material *m_pWireframeMaterial = nullptr;
    GPUBuffer m_LightstyleBuffer;
    GLTexture m_LightstyleTexture;

    unsigned m_uFrameCount = 0;

    // Entities
    std::vector<ClientEntity *> m_SolidEntityList;
    std::vector<ClientEntity *> m_TransEntityList;
    std::vector<size_t> m_SortBuffer;
    unsigned m_uVisibleEntCount = 0;

    // Screen-wide quad
    GLVao m_nQuadVao;
    GPUBuffer m_nQuadVbo;
    void createScreenQuad();

    // Framebuffers
    bool m_bNeedRefreshFB = true;
    glm::ivec2 m_vViewportSize = glm::ivec2(1, 1);
    GLFramebuffer m_nHdrFramebuffer;
    Texture2D m_nColorBuffer;
    RenderBuffer m_nRenderBuffer;

    // Global uniform buffer
    GPUBuffer m_GlobalUniformBuffer;
    GlobalUniform m_GlobalUniform;
    void createGlobalUniform();

    void createLightstyleBuffer();

    void recreateFramebuffer();
    void destroyFramebuffer();

    void asyncCreateSurfaces();
    void asyncLoadBSPLightmaps();
    void finishLoadBSPLightmaps();
    void asyncLoadCustomLightmaps();
    void finishLoadCustomLightmaps();
    void asyncCreateSurfaceObjects();
    void finishCreateSurfaceObjects();
    void updateVao();
    void loadTextures();
    void loadSkyBox();
    void finishLoading();
    std::vector<uint8_t> rotateImage90CW(uint8_t *data, int wide, int tall);
    std::vector<uint8_t> rotateImage90CCW(uint8_t *data, int wide, int tall);

    //! Pre-rendering stuff
    void frameSetup(float flSimTime, float flTimeDelta);

    //! Post-rendering stuff.
    //! Disables stuff enabled in frameSetup.
    void frameEnd();

    //! Binds and clears HDB framebuffer
    void prepareHdrFramebuffer();

    //! Sets view constext settings.
    void setupViewContext();

    //! Binds lightmap block texture
    void bindLightmapBlock();

    //! Updates and binds lightstyle buffer.
    void bindLightstyles();

    //! Draws solid BSP faces.
    void drawWorldSurfaces();

    //! Draws solid BSP faces.
    void drawWorldSurfacesVao();

    //! Draws solid BSP faces using indexed rendering.
    void drawWorldSurfacesIndexed();

    //! Draws BSP faces with SKY texture.
    void drawSkySurfaces();

    //! Draws BSP faces with SKY texture using VAOs.
    void drawSkySurfacesVao();

    //! Draws BSP faces with SKY texture using EBO.
    void drawSkySurfacesIndexed();

    //! Draws solid entities
    void drawSolidEntities();
    void drawSolidTriangles();

    //! Draws transparent entities
    void drawTransEntities();
    void drawTransTriangles();

    //! Draws a solid brush entity
    void drawSolidBrushEntity(ClientEntity *pClent);

    //! Draws a (maybe transparent) brush entity
    void drawBrushEntity(ClientEntity *pClent);

    //! Draws a brush entity surface.
    void drawBrushEntitySurface(Surface &surf);

    //! Draws custom lightmap patches
    void drawPatches();

    //! Post-processes HDB framebuffer (tonemapping, gamma correction) and draws it into active framebuffer.
    void doPostProcessing();

    //! Sets up render mode.
    void setRenderMode(RenderMode mode);
};

#endif
