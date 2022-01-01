#include "rad_sim_impl.h"

#define DISABLE_TREE

namespace {

template <typename T>
[[maybe_unused]] float filterCubic(float x) {
    constexpr float B = T::B;
    constexpr float C = T::C;

    float absx = std::abs(x);
    float val = 0;

    if (absx < 1.0f) {
        val = (12 - 9 * B - 6 * C) * absx * absx * absx;
        val += (-18 + 12 * B + 6 * C) * absx * absx;
        val += 6 - 2 * B;
    } else if (std::abs(x) < 2.0f) {
        val = (-B - 6 * C) * absx * absx * absx;
        val += (6 * B + 30 * C) * absx * absx;
        val += (-12 * B - 48 * C) * absx;
        val += 8 * B + 24 * C;
    } else {
        return 0;
    }

    return val / 6;
}

struct [[maybe_unused]] BSplineArgs {
    static constexpr float B = 1;
    static constexpr float C = 0;
};

// B Spline cubic filter
// https://entropymine.com/imageworsener/bicubic/
[[maybe_unused]] float filterCubicBSpline(float x) {
    return filterCubic<BSplineArgs>(x);
}

struct [[maybe_unused]] MitchellArgs {
    static constexpr float B = 1 / 3.0f;
    static constexpr float C = 1 / 3.0f;
};

// Mitchell cubic filter
// https://entropymine.com/imageworsener/bicubic/
[[maybe_unused]] float filterCubicMitchell(float x) {
    return filterCubic<MitchellArgs>(x);
}

struct [[maybe_unused]] CatRomArgs {
    static constexpr float B = 0;
    static constexpr float C = 0.5f;
};

// Catmull-Rom cubic filter
// https://entropymine.com/imageworsener/bicubic/
[[maybe_unused]] float filterCubicCatRom(float x) {
    return filterCubic<CatRomArgs>(x);
}

// Linear filter 1
[[maybe_unused]] float filterLinear1(float x) {
    if (x < -1) {
        return 0;
    }

    if (x < 0) {
        return 1.0f + x;
    }

    if (x < 1) {
        return 1.0f - x;
    }

    return 0;
}

// Linear filter 2
[[maybe_unused]] float filterLinear2(float x) {
    if (x < -2) {
        return 0;
    }

    if (x < 0) {
        return 1.0f + x / 2.0f;
    }

    if (x < 2) {
        return 1.0f - x / 2.0f;
    }

    return 0;
}

// Filter used for lightmaps
float lightmapFilter(float x) {
    return filterCubicBSpline(x);
}

}

void rad::PatchTree::init(RadSimImpl *pRadSim, Face &face) {
    m_pRadSim = pRadSim;
    m_pFace = &face;
}

void rad::PatchTree::buildTree() {
    // Set up root node
    glm::vec2 faceSize = m_pFace->vFaceMaxs - m_pFace->vFaceMins;
    float squareSize = std::max(m_pRadSim->m_Profile.flLuxelSize, m_pFace->flPatchSize);
    float size = std::max(faceSize.x, faceSize.y);
    m_RootNode.flSize = size + 4.0f * squareSize;
    m_RootNode.vOrigin = m_pFace->vFaceCenter;

#ifndef DISABLE_TREE
    PatchIndex patchCount = m_pFace->iNumPatches; 
    float minSize = m_pFace->flPatchSize * 2.0f;

    for (PatchIndex i = 0; i < patchCount; i++) {
        PatchIndex patchIndex = m_pFace->iFirstPatch + i;
        PatchRef patch(m_pRadSim->m_Patches, patchIndex);
        
        Node *node = &m_RootNode;

        for (;;) {
            int idx = node->getChildForPatch(patch);

            if (idx == -1) {
                node->patches.push_back(patchIndex);
                break;
            } else {
                auto &ptr = node->pChildren[idx];

                if (node->flSize / 2.0f < minSize) {
                    node->patches.push_back(patchIndex);
                    break;
                }

                if (!ptr) {
                    ptr = std::make_unique<Node>();
                    ptr->flSize = node->flSize / 2.0f;
                    ptr->vOrigin = node->getChildOrigin(idx);
                }

                node = ptr.get();
            }
        }
    }
#endif
}

void rad::PatchTree::sampleLight(const glm::vec2 &pos, float radius, float filterk, glm::vec3 &out,
                                 float &weightSum, bool checkTrace) {
    // Check if pos intersects with the face
    glm::vec2 corners[4];
    getCorners(pos, radius * 2.0f, corners);
    glm::vec2 faceAABB[] = {m_pFace->vFaceMins, m_pFace->vFaceMaxs};
    glm::vec2 luxelAABB[] = {corners[0], corners[3]};

    if (!intersectAABB(faceAABB, luxelAABB)) {
        return;
    }

#ifndef DISABLE_TREE

    recursiveSample(&m_RootNode, pos, radius, out, corners, weightSum);

#else
    PatchIndex count = m_pFace->iNumPatches;
    glm::vec3 tracePos = m_pFace->faceToWorld(pos) + m_pFace->vNormal * TRACE_OFFSET;
    // TODO: tracePos should be clamped to face's edge

    for (PatchIndex i = 0; i < count; i++) {
        PatchRef patch(m_pRadSim->m_Patches, m_pFace->iFirstPatch + i);

        glm::vec2 d = patch.getFaceOrigin() - pos;
        glm::vec3 patchTracePos = patch.getOrigin() + patch.getNormal() * TRACE_OFFSET;

        if (std::abs(d.x) <= radius && std::abs(d.y) <= radius &&
            (!checkTrace || m_pRadSim->traceLine(tracePos, patchTracePos) == bsp::CONTENTS_EMPTY)) {
            float weight = lightmapFilter(d.x * filterk) * lightmapFilter(d.y * filterk);
            out += weight * patch.getFinalColor();
            weightSum += weight;
        }
    }
#endif
}

void rad::PatchTree::recursiveSample(Node *node, const glm::vec2 &pos, float radius, glm::vec3 &out,
                                     glm::vec2 corners[4], float &weightSum) {
    // Sample patches
    for (PatchIndex idx : node->patches) {
        PatchRef patch(m_pRadSim->m_Patches, idx);
        glm::vec2 d = patch.getFaceOrigin() - pos;

        if (std::abs(d.x) < radius && std::abs(d.y) < radius) {
            float dist = glm::length(d);
            float weight = patch.getSize() * patch.getSize() / (dist * dist);
            out += weight * patch.getFinalColor();
            weightSum += weight;
        }
    }

    // Check children
    for (int i = 0; i < 4; i++) {
        Node *n = node->pChildren[i].get();

        if (!n) {
            continue;
        }

        glm::vec2 nodeCorners[4];
        getCorners(n->vOrigin, n->flSize, nodeCorners);
        bool needToCheck = false;

        for (int j = 0; j < 4; j++) {
            if (pointIntersectsWithRect(corners[j], n->vOrigin, n->flSize)) {
                needToCheck = true;
                break;
            }
        }

        if (!needToCheck) {
            for (int j = 0; j < 4; j++) {
                if (pointIntersectsWithRect(nodeCorners[j], pos, radius * 2.0f)) {
                    needToCheck = true;
                    break;
                }
            }
        }

        if (needToCheck) {
            recursiveSample(n, pos, radius, out, corners, weightSum);
        }
    }
}

void rad::PatchTree::getCorners(glm::vec2 point, float size, glm::vec2 corners[4]) {
    size = size / 2.0f;
    corners[0] = point + glm::vec2(-size, -size);
    corners[1] = point + glm::vec2(size, -size);
    corners[2] = point + glm::vec2(-size, size);
    corners[3] = point + glm::vec2(size, size);
}

bool rad::PatchTree::pointIntersectsWithRect(glm::vec2 point, glm::vec2 origin, float size) {
    size /= 2.0f;
    return (point.x >= origin.x - size && point.x <= origin.x + size) &&
           (point.y >= origin.y - size && point.y <= origin.y + size);
}

bool rad::PatchTree::intersectAABB(const glm::vec2 b1[2], const glm::vec2 b2[2]) {
    glm::vec2 pos1 = (b1[0] + b1[1]) / 2.0f;
    glm::vec2 pos2 = (b2[0] + b2[1]) / 2.0f;
    glm::vec2 half1 = b1[1] - pos1;
    glm::vec2 half2 = b2[1] - pos2;
    glm::vec2 d = pos2 - pos1;
    return half1.x + half2.x >= std::abs(d.x)
        && half1.y + half2.y >= std::abs(d.y);
}

int rad::PatchTree::Node::getChildForPatch(PatchRef &p) {
    auto fn = [&](const glm::vec2 &pos) {
        bool is0 = pos.x < vOrigin.x && pos.y < vOrigin.y;
        bool is1 = pos.x > vOrigin.x && pos.y < vOrigin.y;
        bool is2 = pos.x < vOrigin.x && pos.y > vOrigin.y;
        bool is3 = pos.x > vOrigin.x && pos.y > vOrigin.y;

        if (is0) {
            AFW_ASSERT(!is1 && !is2 && !is3);
            return 0;
        } else if (is1) {
            AFW_ASSERT(!is2 && !is3);
            return 1;
        } else if (is2) {
            AFW_ASSERT(!is3);
            return 2;
        } else if (is3) {
            return 3;
        } else {
            return -1;
        }
    };

    glm::vec2 org = p.getOrigin();
    float d = p.getSize() / 2.0f;
    int a0 = fn(org + glm::vec2(-d, -d));
    int a1 = fn(org + glm::vec2(d, -d));
    int a2 = fn(org + glm::vec2(-d, d));
    int a3 = fn(org + glm::vec2(d, d));

    if (a0 == a1 && a0 == a2 && a0 == a3) {
        return a0;
    } else {
        return -1;
    }
}

glm::vec2 rad::PatchTree::Node::getChildOrigin(int idx) {
    float d = flSize / 4.0f;

    if (idx == 0) {
        return vOrigin + glm::vec2(-d, -d);
    } else if (idx == 1) {
        return vOrigin + glm::vec2(d, -d);
    } else if (idx == 2) {
        return vOrigin + glm::vec2(-d, d);
    } else {
        return vOrigin + glm::vec2(d, d);
    }
}
