#ifndef RENDERER_SCENE_RENDERER_H
#define RENDERER_SCENE_RENDERER_H
#include <vector>
#include <functional>
#include <bsp/level.h>
#include <app_base/texture_block.h>
#include <graphics/raii.h>
#include <graphics/gpu_buffer.h>
#include <graphics/render_buffer.h>
#include <graphics/framebuffer.h>
#include <graphics/texture2d.h>
#include <material_system/material_system.h>
#include <renderer/client_entity.h>

//! Error for checking on which side of plane a point is.
constexpr float BACKFACE_EPSILON = 0.01f;

//! Maximum count of vertices a surface can have.
constexpr int MAX_SIDE_VERTS = 512;

constexpr int SURF_NOCULL = (1 << 0);    //!< Two-sided polygon (e.g. 'water4b')
constexpr int SURF_PLANEBACK = (1 << 1); //!< Plane should be negated
constexpr int SURF_DRAWSKY = (1 << 2);   //!< Sky surface

class IRendererEngine;

class SceneRenderer : appfw::NoMove {
private:
    struct Surface; // Forward def for ViewContext

public:
    static constexpr int GLOBAL_UNIFORM_BIND = 0;

    enum
    {
        TEX_BRUSH_LIGHTMAP = Material::MAX_TEXTURES,
        TEX_LIGHTSTYLES,
        TEX_MAX_COUNT
    };

    class ViewContext {
    public:
        enum class ProjType
        {
            None = 0,
            Perspective,
            Orthogonal,
        };

        enum class Cull
        {
            None = 0,
            Front,
            Back
        };

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

        //! Returns culling mode.
        inline Cull getCulling() const { return m_Cull; }

        //! Sets which faces to cull.
        inline void setCulling(Cull cull) { m_Cull = cull; }

        //! Returns view position in the world.
        inline const glm::vec3 &getViewOrigin() const { return m_vViewOrigin; }

        //! Returns view forward direction
        inline const glm::vec3 &getViewForward() const { return m_vForward; }

        //! Returns projection matrix.
        inline const glm::mat4 &getProjectionMatrix() const { return m_ProjMat; }

        //! Returns view matrix.
        inline const glm::mat4 &getViewMatrix() const { return m_ViewMat; }

        //! Checks if an AABB is in view frustum.
        //! @returns true if the box is culled and not drawn.
        bool cullBox(glm::vec3 mins, glm::vec3 maxs) const;

        //! Checks if a surface is in view and with correct side.
        //! @returns true if the surface is culled and not drawn.
        bool cullSurface(const Surface &surface) const;

        //! Sets up frustum for current origin and angles. Must be called before usage.
        void setupFrustum();

    private:
        struct Plane {
            glm::vec3 vNormal;
            float fDist;
            uint8_t signbits; // signx + (signy<<1) + (signz<<1)
        };

        ProjType m_Type = ProjType::None;
        Cull m_Cull = Cull::Back;
        glm::mat4 m_ProjMat;
        glm::mat4 m_ViewMat;

        glm::vec3 m_vForward, m_vRight, m_vUp; //!< View direction vectors

        // Perspective options
        glm::vec3 m_vViewOrigin = glm::vec3(0, 0, 0);
        glm::vec3 m_vViewAngles = glm::vec3(0, 0, 0);
        float m_flHorFov = 0;
        float m_flVertFov = 0;
        float m_flAspect = 0;
        float m_flNearZ = 0;
        float m_flFarZ = 0;

        //! Frustum planes
        //! 0 - left
        //! 1 - right
        //! 2 - bottom
        //! 3 - top
        //! 4 - near
        //! 5 - far
        Plane m_Frustum[6];
    };

    SceneRenderer(bsp::Level &level, std::string_view path, IRendererEngine &engine);
    ~SceneRenderer();

    //! Sets size of the viewport.
    void setViewportSize(const glm::ivec2 &size);

    //! Sets the material used for the skybox.
    inline void setSkyboxMaterial(Material *material) { m_pSkyboxMaterial = material; }

    //! @returns the view context.
    inline ViewContext &getViewContext() { return m_ViewContext; }

    //! Renders the image to the framebuffer.
    //! @param  targetFb    Framebuffer to draw to
    //! @param  flSimTime   Current simulation time
    //! @param  flTimeDelta Simulation time since last frame
    void renderScene(GLint targetFb, float flSimTime, float flTimeDelta);

    //! Shows ImGui dialog with debug info.
    void showDebugDialog(const char *title, bool *isVisible = nullptr);

    //! Clears the entity list.
    void clearEntities();

    //! Adds an entity into rendering list.
    //! @returns true if entity was added, false if limit reached
    bool addEntity(ClientEntity *pClent);

    //! @returns the material used by the surface.
    Material *getSurfaceMaterial(int surface);

    //! Sets linear intensity scale of a lightstyle.
    inline void setLightstyleScale(int lightstyle, float scale) {
        m_flLightstyleScales[lightstyle] = scale;
    }

#ifdef RENDERER_SUPPORT_TINTING
    //! Allows tinting world or entity surfaces.
    //! @param  surface The surface index
    //! @param  color   Color in gamma space
    void setSurfaceTint(int surface, glm::vec4 color);
#endif

private:
    class ILightmap;
    class FakeLightmap;
    class BSPLightmap;
    class CustomLightmap;
    class WorldRenderer;
    class BrushRenderer;

    //! Maximum number of vertices. Limited by two byte vertex index in the EBO,
    //! (2^16 - 1) is reserved for primitive restart.
    static constexpr uint16_t MAX_SURF_VERTEX_COUNT = std::numeric_limits<uint16_t>::max() - 1;

    //! Vertex index for primitive restart.
    static constexpr uint16_t PRIMITIVE_RESTART_IDX = std::numeric_limits<uint16_t>::max();

    struct SurfaceVertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texture;
    };

    struct LightmapVertex {
        glm::ivec4 lightstyle;
        glm::vec2 texture;
    };

    struct Surface {
        // Face info
        const bsp::BSPPlane *plane = nullptr;
        int flags = 0;
        glm::vec3 vMins, vMaxs;
        glm::vec3 vOrigin;                   //!< Middle point of the surface (average of vertices)
        std::vector<glm::vec3> faceVertices; //!< World positions of vertices
        Material *material = nullptr;
        unsigned materialIdx = 0;

        // Face info for rendering
        glm::vec3 color;      //!< Unique color of the surface
        int vertexOffset = 0; //!< Index of first vertex in the buffer
        int vertexCount = 0;  //!< Number of vertices
    };

    struct GlobalUniform {
        glm::mat4 mMainProj;
        glm::mat4 mMainView;
        glm::vec4 vMainViewOrigin; // xyz
        glm::vec4 vflParams1;      // x tex gamma, y screen gamma, z sim time, w sim time delta
        glm::vec4 vflParams2;      // x lightmap gamma
        glm::ivec4 viParams1;      // x is texture type, y is lighting type
    };

    struct RenderingStats {
        unsigned uWorldPolys = 0;
        unsigned uSkyPolys = 0;
        unsigned uBrushEntPolys = 0;
        unsigned uDrawCalls = 0;
        unsigned uEboOverflows = 0;
        double flFrameTime = 0;
    };

    bsp::Level &m_Level;
    IRendererEngine &m_Engine;
    fs::path m_CustomLightmapPath;
    RenderingStats m_Stats;

    std::unique_ptr<WorldRenderer> m_pWorldRenderer;
    std::unique_ptr<BrushRenderer> m_pBrushRenderer;

    // Viewport
    glm::ivec2 m_vViewportSize = glm::ivec2(0, 0);
    glm::ivec2 m_vTargetViewportSize = glm::ivec2(800, 600);
    Texture2D m_HdrColorbuffer;
    RenderBuffer m_HdrDepthbuffer;
    Framebuffer m_HdrBackbuffer;
    
    // Shared rendering stuff
    GlobalUniform m_GlobalUniform;
    GPUBuffer m_GlobalUniformBuffer;
    unsigned m_uFrameCount = 0;

    // Brush rendering
    std::unordered_map<Material *, unsigned> m_MaterialIndexes; //!< Maps materials to their unique indexes.
    unsigned m_uNextMaterialIndex = 0;
    std::vector<Surface> m_Surfaces;         //!< BSP surfaces
    GPUBuffer m_SurfaceVertexBuffer;         //!< Surface vertices
    unsigned m_uSurfaceVertexBufferSize = 0; //!< Number of brush vertices
    unsigned m_uMaxEboSize = 0;              //!< Maximum number of elelements in the EBO
    GLVao m_SurfaceVao;
    Material *m_pSkyboxMaterial = nullptr;

#ifdef RENDERER_SUPPORT_TINTING
    GPUBuffer m_SurfaceTintBuffer;
#endif

    // Baked lighting
    std::unique_ptr<FakeLightmap> m_pFakeLightmap;
    std::unique_ptr<BSPLightmap> m_pBSPLightmap;
    std::unique_ptr<CustomLightmap> m_pCustomLightmap;
    ILightmap *m_pCurrentLightmap = nullptr;

    float m_flLightstyleScales[MAX_LIGHTSTYLES] = {};
    GPUBuffer m_LightstyleBuffer;
    GLTexture m_LightstyleTexture;

    // Entities
    std::vector<ClientEntity *> m_SolidEntityList;
    std::vector<ClientEntity *> m_TransEntityList;
    unsigned m_uVisibleEntCount = 0;

    ViewContext m_ViewContext;

    //----------------------------------------------------------------
    // Initialization
    //----------------------------------------------------------------
    //! Initializes m_Surfaces.
    void initSurfaces();

    //! Creates global uniform buffer.
    void createGlobalUniform();

    //! Creates lightstyle buffer texture.
    void createLightstyleBuffer();

    //! Creates surface vertex buffer.
    void createSurfaceBuffers();

    //! Loads lightmaps.
    void loadLightmaps(std::string_view levelPath);

    //! Loads custom lightmap.
    void loadCustomLightmap();

    //! @returns material and its index.
    std::pair<Material *, unsigned> getMaterialForTexture(const bsp::BSPMipTex &miptex);

    //----------------------------------------------------------------
    // Rendering
    //----------------------------------------------------------------
    //! Validates cvars and applies any configuration changes.
    void validateSettings();

    //! Selects a lightmap based on current settings.
    //! @returns the lightmap or nullptr.
    ILightmap *selectLightmap();

    //! Updates the surface vertex array object.
    void updateSurfaceVao();

    //! Sets up OpenGL, updates uniforms
    void frameSetup(float flSimTime, float flTimeDelta);

    //! Called at the end of the rendering
    void frameEnd();

    //! Sets up OpenGL for rendering into the backbuffer
    void viewRenderingSetup();

    //! Reverts viewRenderingSetup
    void viewRenderingEnd();

    //! Renders world polygons
    void drawWorld();

    //! Renders entities
    void drawEntities();

    //! Renders opaque entities
    void drawSolidEntities();

    //! Renders transparent entities
    void drawTransEntities();

    //! Blits the HDR backbuffer into current framebuffer.
    void postProcessBlit();

    //! Sets OpenGL options for a render mode.
    void setRenderMode(RenderMode mode);

    //----------------------------------------------------------------
    // Backbuffer
    //----------------------------------------------------------------
    //! Creates the HDB backbuffer.
    void createBackbuffer();

    //! Destroys all backbuffer-related objects.
    void destroyBackbuffer();
};

#endif
