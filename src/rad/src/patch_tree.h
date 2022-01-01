#ifndef RAD_PATCH_TREE_H
#define RAD_PATCH_TREE_H
#include <appfw/sha256.h>
#include "types.h"

namespace rad {

class RadSim;
class PatchRef;

class PatchTree {
public:
    void init(RadSimImpl *pRadSim, Face &face);
    void buildTree();

    //! Samples light from patches in the square of size radius and center at pos
    //! @returns true if light was sampled (there are patches in that radius)
    void sampleLight(const glm::vec2 &pos, float radius, float filterk, glm::vec3 &out, float &weightSum, bool checkTrace);

private:
    static constexpr float TRACE_OFFSET = 0.1f;

    struct Node {
        //! Size of the node
        float flSize = 0;

        //! Center of the node
        glm::vec2 vOrigin;

        //! Child nodes
        //! x0, y0 - vOrigin
        //! 0: x < x0, y < y0
        //! 1: x > x0, y < y0
        //! 2: x < x0, y > y0
        //! 3: x > x0, y > y0
        std::unique_ptr<Node> pChildren[4];

        //! Patches of this node
        std::vector<PatchIndex> patches;

        //! Returns child index or -1
        int getChildForPatch(PatchRef &p);

        //! Returns child origin for idx
        glm::vec2 getChildOrigin(int idx);
    };

    RadSimImpl *m_pRadSim = nullptr;
    Face *m_pFace = nullptr;
    Node m_RootNode;

    void recursiveSample(Node *node, const glm::vec2 &pos, float radius, glm::vec3 &out,
                         glm::vec2 corners[4], float &weightSum);

    static void getCorners(glm::vec2 point, float size, glm::vec2 corners[4]);
    static bool pointIntersectsWithRect(glm::vec2 point, glm::vec2 origin, float size);
    static bool intersectAABB(const glm::vec2 b1[2], const glm::vec2 b2[2]);
};

}

#endif
