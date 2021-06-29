#include <appfw/appfw.h>
#include "bsp_tree.h"
#include "bspviewer.h"

BSPTree g_BSPTree;

static glm::vec3 s_Points[2];

static ConCommand cmd_trace1("trace1", "", []() {
    s_Points[0] = BSPViewer::get().getCameraPos();
    printi("Trace: point 1 ({}; {}; {})", s_Points[0].x, s_Points[0].y, s_Points[0].z);
});

static ConCommand cmd_trace2("trace2", "", []() {
    s_Points[1] = BSPViewer::get().getCameraPos();
    printi("Trace: point 2 ({}; {}; {})", s_Points[1].x, s_Points[1].y, s_Points[1].z);

    printi("Trace: distance {}", glm::length(s_Points[1] - s_Points[0]));
    bsp::Level *lvl = g_BSPTree.getLevel();
    int traceRes = lvl->traceLine(s_Points[0], s_Points[1]);
    if (traceRes != bsp::CONTENTS_EMPTY) {
        printe("HIT {}", traceRes);
    } else {
        printw("CLEAR");
    }
});

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
    pPlane = &BSPTree::m_pLevel->getPlanes().at(bspNode.iPlane);
    pChildren[0] = pChildren[1] = nullptr;
    //faces = appfw::span(BSPTree::m_pLevel->getFaces()).subspan(bspNode.firstFace, bspNode.nFaces);
}

BSPTree::Leaf::Leaf(const bsp::BSPLeaf &bspLeaf) {
    AFW_ASSERT(bspLeaf.nContents < 0);
    nContents = bspLeaf.nContents;

    /*if (bspLeaf.nVisOffset != -1 && !g_Level.getVisData().empty()) {
        pCompressedVis = &g_Level.getVisData().at(bspLeaf.nVisOffset);
    }*/
}

void BSPTree::createTree() {
    printd("Loading BSP tree");
    m_Leaves.clear();
    m_Nodes.clear();
    createLeaves();
    createNodes();
}

bool BSPTree::traceLine(glm::vec3 from, glm::vec3 to) {
    return recursiveTraceLine(&m_Nodes[0], from, to);
}

void BSPTree::createLeaves() {
    AFW_ASSERT(m_Leaves.empty());
    auto &lvlLeaves = BSPTree::m_pLevel->getLeaves();

    m_Leaves.reserve(lvlLeaves.size());

    for (size_t i = 0; i < lvlLeaves.size(); i++) {
        m_Leaves.emplace_back(lvlLeaves[i]);
    }
}

void BSPTree::createNodes() {
    AFW_ASSERT(m_Nodes.empty());
    auto &lvlNodes = BSPTree::m_pLevel->getNodes();

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
bool BSPTree::recursiveTraceLine(NodeBase *pNode, const glm::vec3 &start, const glm::vec3 &stop) {
    if (pNode->nContents < 0) {
        if (pNode->nContents == bsp::CONTENTS_SOLID) {
            return true;
        } else {
            return false;
        }
    }

    Node *pRealNode = static_cast<Node *>(pNode);

#if 0
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

#if 1
    // Code from qrad/trace.c TestLine_r
    constexpr float ON_EPSILON = 0.01f;
    float front, back;
    switch (pRealNode->pPlane->nType) {
    case bsp::PlaneType::PlaneX:
        front = start[0] - pRealNode->pPlane->fDist;
        back = stop[0] - pRealNode->pPlane->fDist;
        break;
    case bsp::PlaneType::PlaneY:
        front = start[1] - pRealNode->pPlane->fDist;
        back = stop[1] - pRealNode->pPlane->fDist;
        break;
    case bsp::PlaneType::PlaneZ:
        front = start[2] - pRealNode->pPlane->fDist;
        back = stop[2] - pRealNode->pPlane->fDist;
        break;
    default:
        front = (start[0] * pRealNode->pPlane->vNormal[0] + start[1] * pRealNode->pPlane->vNormal[1] +
                 start[2] * pRealNode->pPlane->vNormal[2]) -
                pRealNode->pPlane->fDist;
        back = (stop[0] * pRealNode->pPlane->vNormal[0] + stop[1] * pRealNode->pPlane->vNormal[1] +
                stop[2] * pRealNode->pPlane->vNormal[2]) -
               pRealNode->pPlane->fDist;
        break;
    }

    if (front >= -ON_EPSILON && back >= -ON_EPSILON)
        return recursiveTraceLine(pRealNode->pChildren[0], start, stop);

    if (front < ON_EPSILON && back < ON_EPSILON)
        return recursiveTraceLine(pRealNode->pChildren[1], start, stop);

    int side = front < 0;

    float frac = front / (front - back);

    glm::vec3 mid;
    mid[0] = start[0] + (stop[0] - start[0]) * frac;
    mid[1] = start[1] + (stop[1] - start[1]) * frac;
    mid[2] = start[2] + (stop[2] - start[2]) * frac;

    bool r = recursiveTraceLine(pRealNode->pChildren[side], start, mid);
    if (r)
        return r;
    return recursiveTraceLine(pRealNode->pChildren[!side], mid, stop);
#endif
}
