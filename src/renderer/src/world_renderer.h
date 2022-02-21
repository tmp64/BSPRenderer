#ifndef WORLD_RENDERER_H
#define WORLD_RENDERER_H
#include <renderer/scene_renderer.h>

class SceneRenderer::WorldRenderer {
public:
    struct WorldSurfaceList {
        std::vector<std::vector<unsigned>> textureChain;
        std::vector<unsigned> textureChainFrames;
        std::vector<unsigned> skySurfaces;
        unsigned textureChainFrame = 0;

        unsigned visFrame = 0;              //!< Current visframe
        int viewLeaf = 0;                   //!< Index of view origin leaf
        std::vector<unsigned> nodeVisFrame; //!< Visframes of nodes
        std::vector<unsigned> leafVisFrame; //!< Visframes of leaves
    };

    WorldRenderer(SceneRenderer &renderer);

    //! Puts visible surfaces into surfList. Make sure to keep it to reduce reallocations.
    //! surfList.textureChain[i] contains a list of surfaces with texture i.
    //! surfList.textureChainFrames[i] contains the frame at which it was drawn.
    //! Texture chain i must only be drawn if its frame == surfList.textureChainFrame.
    //! surfList.skySurfaces contains list of sky surfaces.
    void getTexturedWorldSurfaces(ViewContext &context, WorldSurfaceList &surfList) const;

    //! Renders the world, performs batching.
    void drawTexturedWorld(WorldSurfaceList &surfList);

    //! Renders the sky polygons in one batch.
    void drawSkybox(WorldSurfaceList &surfList);
    
    //! Renders world and sky polygons as wireframe.
    void drawWireframe(WorldSurfaceList &surfList, bool drawSky);

    //! Surf list for main view
    WorldSurfaceList m_MainWorldSurfList;

private:
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

    // World info
    SceneRenderer &m_Renderer;
    std::vector<Node> m_Nodes;
    std::vector<Leaf> m_Leaves;

    // Rendering info
    GPUBuffer m_WorldEbo;
    std::vector<uint16_t> m_WorldEboBuf;

    void createLeaves();
    void createNodes();
    void updateNodeParents(int iNode, int parent);
    void createBuffers();

    //! Goes over all visible leaves and nodes and sets their visframes to current visframe.
    void markLeaves(ViewContext &context, WorldSurfaceList &surfList) const;

    //! Traverses BSP tree and puts visible surfaces into the surf list. Order is front to back.
    void recursiveWorldNodesTextured(ViewContext &context, WorldSurfaceList &surfList,
                                     int node) const;
};

#endif
