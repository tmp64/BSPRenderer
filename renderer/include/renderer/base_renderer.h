#ifndef RENDERER_BASE_RENDERER_H
#define RENDERER_BASE_RENDERER_H
#include <array>
#include <appfw/utils.h>
#include <appfw/console/con_item.h>
#include <bsp/level.h>

extern appfw::console::ConVar<int> r_cull;
extern appfw::console::ConVar<bool> r_drawworld;

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
    std::vector<glm::vec3> vertices;
};

struct LevelNodeBase {
    int nContents = 0;
    unsigned iVisFrame = 0;
    LevelNodeBase *pParent = nullptr;
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

class BaseRenderer : appfw::utils::NoCopy {
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
         * Near clipping plane
         */
        float near = 8.f;

        /**
         * Fra clipping plane
         */
        float far = 8192.f;
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
     */
    void setLevel(const bsp::Level *level);

    /**
     * Draws the world onto the screen.
     * @param   options How to draw
     * @return Draw statistics
     */
    DrawStats draw(const DrawOptions &options) noexcept;

protected:
    std::vector<std::unique_ptr<LevelLeaf>> m_Leaves;
    std::vector<std::unique_ptr<LevelNode>> m_Nodes;
    std::vector<LevelSurface> m_BaseSurfaces;
    DrawStats m_DrawStats;

    inline const DrawOptions &getOptions() { return *m_pOptions; }

    /**
     * Creates an instance of LevelLeaf. It may be extended by renderers.
     */
    virtual LevelLeaf *createLeaf(const bsp::BSPLeaf &bspLeaf);

    /**
     * Creates an instance of LevelNode. It may be extended by renderers.
     */
    virtual LevelNode *createNode(const bsp::BSPNode &bspNode);

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

private:
    const DrawOptions *m_pOptions = nullptr;
    const bsp::Level *m_pLevel = nullptr;

    // Level loading
    void createBaseSurfaces();
    void createLeaves();
    void createNodes();
    void updateNodeParents(LevelNodeBase *node, LevelNodeBase *parent);

    // Rendering
    std::vector<size_t> m_WorldSurfacesToDraw;
    std::array<uint8_t, bsp::MAX_MAP_LEAFS / 8> m_DecompressedVis;
    LevelLeaf *m_pViewLeaf = nullptr;
    LevelLeaf *m_pOldViewLeaf = nullptr;
    unsigned m_iFrame = 0;
    unsigned m_iVisFrame = 0;

    // Visibility
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

    void drawWorld() noexcept;
    void recursiveWorldNodes(LevelNodeBase *pNodeBase) noexcept;
};

#endif
