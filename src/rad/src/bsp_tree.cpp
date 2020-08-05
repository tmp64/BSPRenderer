#include <appfw/services.h>
#include "bsp_tree.h"
#include "utils.h"

BSPTree g_BSPTree;

static inline float planeDiff(glm::vec3 point, const bsp::BSPPlane &plane) {
    float res = 0;

    switch (plane.nType) {
    case bsp::PlaneType::PlaneX:
        res = point[0];
        break;
    case bsp::PlaneType::PlaneY:
        res = point[1];
        break;
    case bsp::PlaneType::PlaneZ:
        res = point[2];
        break;
    default:
        res = glm::dot(point, plane.vNormal);
        break;
    }

    return res - plane.fDist;
}

BSPTree::Node::Node(const bsp::BSPNode &bspNode) {
    pPlane = &g_Planes.at(bspNode.iPlane);
    pChildren[0] = pChildren[1] = nullptr;
    faces = appfw::span(g_Faces).subspan(bspNode.firstFace, bspNode.nFaces);
}

BSPTree::Leaf::Leaf(const bsp::BSPLeaf &bspLeaf) {
    AFW_ASSERT(bspLeaf.nContents < 0);
    nContents = bspLeaf.nContents;

    if (bspLeaf.nVisOffset != -1 && !g_Level.getVisData().empty()) {
        pCompressedVis = &g_Level.getVisData().at(bspLeaf.nVisOffset);
    }
}

void BSPTree::createTree() {
    logDebug("Loading BSP tree");
    createLeaves();
    createNodes();
}

bool BSPTree::traceLine(glm::vec3 from, glm::vec3 to) {
    return recursiveTraceLine(&m_Nodes[0], from, to);
}

void BSPTree::createLeaves() {
    AFW_ASSERT(m_Leaves.empty());
    auto &lvlLeaves = g_Level.getLeaves();

    m_Leaves.reserve(lvlLeaves.size());

    for (size_t i = 0; i < lvlLeaves.size(); i++) {
        m_Leaves.emplace_back(lvlLeaves[i]);
    }
}

void BSPTree::createNodes() {
    AFW_ASSERT(m_Nodes.empty());
    auto &lvlNodes = g_Level.getNodes();

    m_Nodes.reserve(lvlNodes.size());

    for (size_t i = 0; i < lvlNodes.size(); i++) {
        m_Nodes.emplace_back(lvlNodes[i]);
    }

    // Set up children
    for (size_t i = 0; i < lvlNodes.size(); i++) {
        for (int j = 0; j < 2; j++) {
            int childNodeIdx = lvlNodes[i].iChildren[j];

            if (childNodeIdx >= 0) {
                m_Nodes[i].pChildren[j] = &m_Nodes.at(childNodeIdx);
            } else {
                childNodeIdx = ~childNodeIdx;
                m_Nodes[i].pChildren[j] = &m_Leaves.at(childNodeIdx);
            }
        }
    }

    updateNodeParents(&m_Nodes[0], nullptr);
}

void BSPTree::updateNodeParents(NodeBase *node, NodeBase *parent) {
    node->pParent = parent;

    if (node->nContents < 0) {
        return;
    }

    Node *realNode = static_cast<Node *>(node);
    updateNodeParents(realNode->pChildren[0], node);
    updateNodeParents(realNode->pChildren[1], node);
}

// Based on VDC article: https://developer.valvesoftware.com/wiki/BSP#How_are_BSP_trees_used_for_collision_detection.3F
// and qrad code from HLSDK.
bool BSPTree::recursiveTraceLine(NodeBase *pNode, const glm::vec3 &from, const glm::vec3 &to) {
    if (pNode->nContents < 0) {
        if (pNode->nContents == bsp::CONTENTS_SOLID) {
            return true;
        } else {
            return false;
        }
    }

    Node *pRealNode = static_cast<Node *>(pNode);

#if 1
    // Code from VDC
    // Checks incorrectly
    int fromSide = (planeDiff(from, *pRealNode->pPlane) >= 0.0f) ? 0 : 1;
    int toSide = (planeDiff(from, *pRealNode->pPlane) >= 0.0f) ? 0 : 1;

    if (fromSide == toSide) {
        return recursiveTraceLine(pRealNode->pChildren[fromSide], from, to);
    }

    bool res1 = recursiveTraceLine(pRealNode->pChildren[fromSide], from, to);
    bool res2 = recursiveTraceLine(pRealNode->pChildren[toSide], from, to);

    if (res1 || res2) {
        return true;
    } else {
        return false;
    }
#endif

#if 0
    // Code from qrad/trace.c TestLine_r
    // Appears to always return false.
    float front = planeDiff(from, *pRealNode->pPlane);
    float back = planeDiff(from, *pRealNode->pPlane);

    if (front >= -ON_EPSILON && back >= -ON_EPSILON)
        return recursiveTraceLine(pRealNode->pChildren[0], from, to);

    if (front < ON_EPSILON && back < ON_EPSILON)
        return recursiveTraceLine(pRealNode->pChildren[0], from, to);

    bool side = front < 0;
    float frac = front / (front - back);

    glm::vec3 mid;
    mid[0] = from[0] + (to[0] - from[0]) * frac;
    mid[1] = from[1] + (to[1] - from[1]) * frac;
    mid[2] = from[2] + (to[2] - from[2]) * frac;

    bool res = recursiveTraceLine(pRealNode->pChildren[side], from, mid);
    if (res) {
        return res;
    }

    return recursiveTraceLine(pRealNode->pChildren[!side], mid, to);
#endif
}
