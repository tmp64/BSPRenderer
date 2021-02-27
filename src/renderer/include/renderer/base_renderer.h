#ifndef RENDERER_BASE_RENDERER_H
#define RENDERER_BASE_RENDERER_H
#include <array>
#include <appfw/utils.h>
#include <appfw/console/con_item.h>
#include <bsp/level.h>
#include <renderer/material_manager.h>

extern ConVar<int> r_cull;
extern ConVar<bool> r_drawworld;
extern ConVar<int> r_fullbright;
extern ConVar<float> r_gamma;
extern ConVar<float> r_texgamma;

constexpr float BACKFACE_EPSILON = 0.01f;

/**
 * Maximum count of vertices a surface can have.
 */
constexpr int MAX_SIDE_VERTS = 512;

/**
 * Two-sided polygon (e.g. 'water4b')
 */
constexpr int SURF_NOCULL = (2 << 0);

/**
 * Plane should be negated
 */
constexpr int SURF_PLANEBACK = (2 << 0);

struct LevelSurface {
    int iFirstEdge = 0;
    int iNumEdges = 0;
    int iFlags = 0;
    const bsp::BSPPlane *pPlane = nullptr;
    const bsp::BSPTextureInfo *pTexInfo = nullptr;
    size_t nMatIndex = NULL_MATERIAL;
    std::vector<glm::vec3> vertices;
    glm::vec3 mins, maxs;

    // Lightmap info
    std::vector<glm::vec2> lmTexCoords;
    glm::ivec2 lmSize;
    GLuint nLightmapTex = 0;
};

struct LevelNodeBase {
    int nContents = 0;
    unsigned iVisFrame = 0;
    LevelNodeBase *pParent = nullptr;
    virtual ~LevelNodeBase() = default;
};

struct LevelNode : public LevelNodeBase {
    const bsp::BSPPlane *pPlane = nullptr;
    LevelNodeBase *children[2];
    int iFirstSurface = 0;
    int iNumSurfaces = 0;

    LevelNode(const bsp::Level &level, const bsp::BSPNode &bspNode);
};

struct LevelLeaf : public LevelNodeBase {
    const uint8_t *pCompressedVis = nullptr;

    LevelLeaf(const bsp::Level &level, const bsp::BSPLeaf &bspLeaf);
};

struct Plane : public bsp::BSPPlane {
    uint8_t signbits; // signx + (signy<<1) + (signz<<1)
};

class BaseRenderer : appfw::NoCopy {
public:
    /**
     * Contains rendering settings.
     */
    struct DrawOptions {
        /**
         * Location of the camera.
         */
        glm::vec3 viewOrigin;

        /**
         * Rotation of the camera (pitch, yaw, roll).
         */
        glm::vec3 viewAngles;

        /**
         * Horizontal field of view.
         */
        float fov = 90;

        /**
         * Aspect ratio = W / H
         */
        float aspect = 1;

        /**
         * Near clipping plane (minimum is 4 units)
         */
        float near = 8.f;

        /**
         * Far clipping plane
         */
        float far = 8192.f;
    };

    struct LevelVars {
        std::string lightmapPath;
        std::vector<LevelLeaf> leaves;
        std::vector<LevelNode> nodes;
        std::vector<LevelSurface> baseSurfaces;
        bool bLightmapLoaded = false;

        LevelLeaf *pOldViewLeaf = nullptr;
        unsigned iFrame = 0;
        unsigned iVisFrame = 0;
    };

    struct FrameVars {
        float fov_x = 90;
        float fov_y = 90;
        glm::vec3 viewOrigin;
        glm::vec3 viewAngles;
        glm::mat4 viewMat;
        glm::mat4 projMat;
        glm::vec3 vForward, vRight, vUp;
        float flViewPlaneDist = 0;
        std::array<Plane, 6> frustum;

        std::vector<size_t> worldSurfacesToDraw;

        std::array<uint8_t, bsp::MAX_MAP_LEAFS / 8> decompressedVis;
        LevelLeaf *pViewLeaf = nullptr;
    };

    /**
     * Contains rendering statistics.
     */
    struct DrawStats {
        /**
         * Count of world surfaces drawn on the screen.
         */
        size_t worldSurfaces = 0;
    };

    BaseRenderer();
    virtual ~BaseRenderer();

    /**
     * Whether or not level is loaded.
     */
    inline bool isLevelLoaded() { return m_pLevel; }

    /**
     * Returns the level being rendered.
     * Safe to call only if isLevelLoaded() == true.
     */
    inline const bsp::Level &getLevel() const {
        return *m_pLevel;
    }

    /**
     * Sets which level to draw. It must exist for as long as it is set and must not change.
     * On error, throws and leaves no map loaded.
     * @param   level   Level or nullptr
     * @param   mapPath Path to .bsp file (used for lightmap loading)
     */
    void setLevel(const bsp::Level *level, const std::string &mapPath = "");

    /**
     * Reloads lightmap.
     */
    void reloadLightmap();

    /**
     * Draws the world onto the screen.
     * @param   options How to draw
     * @return Draw statistics
     */
    DrawStats draw(const DrawOptions &options) noexcept;

    void drawGui(const DrawStats &stats);

    virtual void updateScreenSize(glm::ivec2 size);

protected:
    glm::ivec2 m_ScreenSize = {1, 1};
    DrawStats m_DrawStats;

    inline LevelVars &getLevelVars() { return m_LevelVars; }
    inline FrameVars &getFrameVars() { return m_FrameVars; }

    /**
     * Creates surfaces based on data from m_BaseSurfaces.
     */
    virtual void createSurfaces() = 0;

    /**
     * Destroys surfaces created in createSurfaces.
     */
    virtual void destroySurfaces() = 0;

    /**
     * Draws static world surfaces. Order is undefined.
     * @param   surfacesIdx List of surfaces that need to be drawn.
     */
    virtual void drawWorldSurfaces(const std::vector<size_t> &surfaceIdxs) noexcept = 0;

    virtual void drawChildGui(const DrawStats &stats);

private:
    const DrawOptions *m_pOptions = nullptr;
    const bsp::Level *m_pLevel = nullptr;

    /**
     * Variables that are only valid for a level.
     */
    LevelVars m_LevelVars;

    /**
     * Variables that are updated every frame.
     */
    FrameVars m_FrameVars;

    //----------------------------------------------------------------
    // Level loading
    //----------------------------------------------------------------
    void createBaseSurfaces();
    void createLeaves();
    void createNodes();
    void updateNodeParents(LevelNodeBase *node, LevelNodeBase *parent);
    void loadLightmapFile(const std::string &filepath);
    void cleanUpLightmaps();

    //----------------------------------------------------------------
    // Visibility calculations
    //----------------------------------------------------------------
    /**
     * Returns true if the box is completely outside the frustum
     */
    bool cullBox(glm::vec3 mins, glm::vec3 maxs) noexcept;

    /**
     * Returns leaf in which the point is located.
     */
    LevelLeaf *pointInLeaf(glm::vec3 p) noexcept;

    /**
     * Returns decompressed PVS data for a leaf.
     */
    uint8_t *leafPVS(LevelLeaf *pLeaf) noexcept;

    /**
     * Marks surfaces in PVS.
     */
    void markLeaves() noexcept;

    /**
     * Culls invisible surfaces.
     * @return true if surface was culled and shouldn't be drawn.
     */
    bool cullSurface(const LevelSurface &pSurface) noexcept;

    //----------------------------------------------------------------
    // Rendering
    //----------------------------------------------------------------
    void setupFrame() noexcept;
    void drawWorld() noexcept;
    void recursiveWorldNodes(LevelNodeBase *pNodeBase) noexcept;
};

#endif
