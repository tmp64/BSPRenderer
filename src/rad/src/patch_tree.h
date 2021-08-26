#ifndef RAD_PATCH_TREE_H
#define RAD_PATCH_TREE_H
#include <appfw/sha256.h>
#include "types.h"

namespace rad {

class RadSim;
class PatchRef;

class PatchTree {
public:
    void createPatches(RadSimImpl *pRadSim, Face &face, std::atomic<PatchIndex> &globalPatchCount,
                       float flMinPatchSize);
    PatchIndex copyPatchesToTheList(PatchIndex offset, appfw::SHA256 &hash);
    void buildTree();

    //! Samples light from patches in the square of size radius and center at pos
    //! @returns true if light was sampled (there are patches in that radius)
    void sampleLight(const glm::vec2 &pos, float radius, float filterk, glm::vec3 &out, float &weightSum);

private:
    enum class PatchPos
    {
        Inside,          //!< Patch is completely inside the face
        PartiallyInside, //!< Patch is partially inside and outside
        Outside          //!< Patch is completely outside the face
    };

    struct MiniPatch {
        MiniPatch *pNext = nullptr;
        float flSize = 0;

        //! Face coords of the lower corner of the patch (not the center!)
        glm::vec2 vFaceOrigin;
    };

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
    MiniPatch *m_pPatches = nullptr;
    Node m_RootNode;

    //! Divides the patch
    //! @return number of created patches
    PatchIndex subdividePatch(MiniPatch *patch, MiniPatch *&newFirstPatch, MiniPatch *&newLastPatch,
                        bool &keepPatch, float flMinPatchSize) noexcept;

    void recursiveSample(Node *node, const glm::vec2 &pos, float radius, glm::vec3 &out,
                         glm::vec2 corners[4], float &weightSum);

    PatchPos checkPatchPos(MiniPatch *patch) noexcept;

    bool isInFace(glm::vec2 point) noexcept;

    static void getCorners(glm::vec2 point, float size, glm::vec2 corners[4]);
    static bool pointIntersectsWithRect(glm::vec2 point, glm::vec2 origin, float size);
};

}

#endif
