#ifndef BSP_TREE_H
#define BSP_TREE_H
#include <bsp/level.h>

class BSPTree {
public:
    struct NodeBase {
        int nContents = 0;
        NodeBase *pParent = nullptr;

        virtual ~NodeBase() = default;
    };

    struct Node : public NodeBase {
        const bsp::BSPPlane *pPlane = nullptr;
        NodeBase *pChildren[2];
        appfw::span<bsp::BSPFace> faces;

        Node(const bsp::BSPNode &bspNode);
    };

    struct Leaf : public NodeBase {
        const uint8_t *pCompressedVis = nullptr;

        Leaf(const bsp::BSPLeaf &bspLeaf);
    };

    /**
     * Loads BSP from loaded level.
     */
    void createTree();

    /**
     * Traces a line and returns true if hit something.
     * WARNING: Doesn't work properly, requires further testing.
     */
    bool traceLine(glm::vec3 from, glm::vec3 to);

    static inline void setLevel(bsp::Level *lvl) { m_pLevel = lvl; }

private:
    static inline bsp::Level *m_pLevel = nullptr;
    std::vector<Leaf> m_Leaves;
    std::vector<Node> m_Nodes;

    void createLeaves();
    void createNodes();
    void updateNodeParents(NodeBase *node, NodeBase *parent);

    bool recursiveTraceLine(NodeBase *pNode, const glm::vec3 &from, const glm::vec3 &to);
};

extern BSPTree g_BSPTree;

#endif
