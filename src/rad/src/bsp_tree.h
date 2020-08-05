#ifndef BSP_TREE_H
#define BSP_TREE_H
#include "main.h"

class BSPTree {
public:
    struct NodeBase {
        int nContents = 0;
        NodeBase *pParent = nullptr;

        virtual ~NodeBase() = default;
    };

    struct Node : public NodeBase {
        const Plane *pPlane = nullptr;
        NodeBase *pChildren[2];
        appfw::span<Face> faces;

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

private:
    std::vector<Leaf> m_Leaves;
    std::vector<Node> m_Nodes;

    void createLeaves();
    void createNodes();
    void updateNodeParents(NodeBase *node, NodeBase *parent);

    bool recursiveTraceLine(NodeBase *pNode, const glm::vec3 &from, const glm::vec3 &to);
};

extern BSPTree g_BSPTree;

#endif
