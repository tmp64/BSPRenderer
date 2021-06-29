#ifndef RENDERER_SURFACE_RENDERER_H
#define RENDERER_SURFACE_RENDERER_H
#include <appfw/utils.h>
#include <appfw/appfw.h>
#include <bsp/level.h>
#include <glm/glm.hpp>
#include <renderer/material_manager.h>

/**
 * Error for checking on which side of plane a point is.
 */
constexpr float BACKFACE_EPSILON = 0.01f;

/**
 * Maximum count of vertices a surface can have.
 */
constexpr int MAX_SIDE_VERTS = 512;

/**
 * Two-sided polygon (e.g. 'water4b')
 */
constexpr int SURF_NOCULL = (1 << 0);

/**
 * Plane should be negated
 */
constexpr int SURF_PLANEBACK = (1 << 1);

/**
 * Sky surface
 */
constexpr int SURF_DRAWSKY = (1 << 2);

class SurfaceRenderer : appfw::NoCopy {
public:
    enum class ProjType {
        None = 0,
        Perspective,
        Orthogonal,
    };

    enum class Cull {
        None = 0,
        Front,
        Back
    };

    struct Plane : public bsp::BSPPlane {
        uint8_t signbits; // signx + (signy<<1) + (signz<<1)
    };

    class Context {
    public:
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
         * Returns culling mode.
         */
        inline Cull getCulling() { return m_Cull; }

        /**
         * Sets which faces to cull.
         */
        inline void setCulling(Cull cull) { m_Cull = cull; }

        /**
         * Returns view position in the world.
         */
        inline const glm::vec3 &getViewOrigin() { return m_vViewOrigin; }

        /**
         * Returns view forward direction
         */
        inline const glm::vec3 &getViewForward() { return m_vForward; }

        /**
         * Returns projection matrix.
         */
        inline const glm::mat4 &getProjectionMatrix() { return m_ProjMat; }

        /**
         * Returns view matrix.
         */
        inline const glm::mat4 &getViewMatrix() { return m_ViewMat; }

        /**
         * Returns list of texture chains. Each chain needs to be rendered only if
         * its frame == getWorldTextureChainFrame().
         * In that case it definitely has at least one surface.
         */
        inline std::vector<std::vector<unsigned>> &getWorldTextureChain() { return m_WorldTextureChain; }

        /**
         * Returns frames if texture chains.
         */
        inline std::vector<unsigned> &getWorldTextureChainFrames() { return m_WorldTextureChainFrames; }

        /**
         * Returns current world texture chain frame.
         */
        inline unsigned getWorldTextureChainFrame() { return m_uWorldTextureChainFrame; }

        /**
         * Returns list of sky surfaces that are visible and need to be rendered.
         */
        inline std::vector<unsigned> &getSkySurfaces() { return m_SkySurfaces; }

    private:
        ProjType m_Type = ProjType::None;
        Cull m_Cull = Cull::Back;
        glm::mat4 m_ProjMat;
        glm::mat4 m_ViewMat;

        // Perspective options
        glm::vec3 m_vViewOrigin = glm::vec3(0, 0, 0);
        glm::vec3 m_vViewAngles = glm::vec3(0, 0, 0);
        float m_flHorFov = 0;
        float m_flVertFov = 0;
        float m_flAspect = 0;
        float m_flNearZ = 0;
        float m_flFarZ = 0;

        // Rendering
        unsigned m_uWorldTextureChainFrame = 0;
        std::vector<std::vector<unsigned>> m_WorldTextureChain;
        std::vector<unsigned> m_WorldTextureChainFrames;
        std::vector<unsigned> m_SkySurfaces;

        // PVS culling
        unsigned m_iVisFrame = 0;   // Current visframe
        int m_iViewLeaf = 0;        // Leaf for which PVS visibility is calculated (0 if none)
        std::vector<unsigned> m_NodeVisFrame;   // Visible if [i] == m_iVisFrame
        std::vector<unsigned> m_LeafVisFrame;
        std::array<uint8_t, bsp::MAX_MAP_LEAFS / 8> m_DecompressedVis;  // Filled by leafPVS

        // Frustum culling
        glm::vec3 m_vForward, m_vRight, m_vUp;    // View direction vectors

        /*
         * 0 - left
         * 1 - right
         * 2 - bottom
         * 3 - top
         * 4 - near
         * 5 - far
         */
        std::array<Plane, 6> m_Frustum;

        void setupFrustum();    // Sets up view frustum

        friend class SurfaceRenderer;
    };

    struct Surface {
        int iFirstEdge = 0;
        int iNumEdges = 0;

        int iFlags = 0;
        const bsp::BSPPlane *pPlane = nullptr;

        const bsp::BSPTextureInfo *pTexInfo = nullptr;
        size_t nMatIndex = NULL_MATERIAL;

        std::vector<glm::vec3> vVertices;
        glm::vec3 vMins, vMaxs;
        glm::vec3 vOrigin; //!< Middle point of the surface (averga of verticies)

        uint32_t nLightmapOffset = bsp::NO_LIGHTMAP_OFFSET;
    };

    struct BaseNode {
        int nContents = 0;
        unsigned iParentIdx = 0;
    };

    struct Node : public BaseNode {
        unsigned iFirstSurface = 0;
        unsigned iNumSurfaces = 0;
        const bsp::BSPPlane *pPlane = nullptr;
        unsigned iChildren[2];
    };

    struct Leaf : public BaseNode {
        const uint8_t *pCompressedVis = nullptr;
    };

    struct LevelData {
        std::vector<Node> nodes;
        std::vector<Leaf> leaves;
        std::vector<Surface> surfaces;
        size_t nMaxMaterial = 0;
    };

    SurfaceRenderer();

    /**
     * Returns level set to the renderer.
     */
    inline const bsp::Level *getLevel() { return m_pLevel; }

    /**
     * Sets the level.
     */
    void setLevel(const bsp::Level *level);

    /**
     * Returns context set to the renderer.
     */
    inline Context *getContext() { return m_pContext; }

    /**
     * Sets the context.
     */
    inline void setContext(Context *context) { m_pContext = context; }

    /**
     * Returns numver of surfaces.
     */
    inline size_t getSurfaceCount() { return m_Data.surfaces.size(); }

    /**
     * Returns a surface reference.
     */
    inline const Surface &getSurface(size_t i) { return m_Data.surfaces[i]; }

    /**
     * Calculates visible world surfaces and puts them into the context.
     * Use Context::getWorldSurfaces to get them.
     */
    void calcWorldSurfaces();

    /**
     * Returns true if surface shouldn't be drawn.
     */
    bool cullSurface(const Surface &surface) noexcept;

    /**
     * Returns true if an AABB is completely outside the frustum
     */
    bool cullBox(glm::vec3 mins, glm::vec3 maxs) noexcept;

    /**
     * Finds leaf which contains the point.
     * @return  Negative int pointing to the leaf.
     */
    int pointInLeaf(glm::vec3 p) noexcept;

    /**
     * Decompresses (RLE) PVS data for a leaf.
     */
    uint8_t *leafPVS(int leaf) noexcept;

private:
    const bsp::Level *m_pLevel = nullptr;
    Context *m_pContext = nullptr;
    LevelData m_Data;

    /**
     * Fills m_Data.surfaces with BSP faces.
     */
    void createSurfaces();

    /**
     * Filles m_Data.leaves with BSP leaves.
     */
    void createLeaves();

    /**
     * Filles m_Data.nodes with BSP nodes.
     */
    void createNodes();

    /**
     * Recursively updates node parents.
     */
    void updateNodeParents(int node, int parent);

    /**
     * Marks which nodes and leafs are in current PVS.
     */
    void markLeaves() noexcept;

    /**
     * Walks over the BSP tree and fills surface list with visible surfaces. Front surfaces first.
     */
    void recursiveWorldNodes(int node) noexcept;
};

#endif
