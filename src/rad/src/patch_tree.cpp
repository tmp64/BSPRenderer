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

void rad::PatchTree::createPatches(RadSimImpl *pRadSim, Face &face,
                                   std::atomic<PatchIndex> &globalPatchCount,
                                   float flMinPatchSize) {
    m_pRadSim = pRadSim;
    m_pFace = &face;

    // Calculate base size of patch grid (wide x tall patches of flPatchSize)
    glm::vec2 faceSize = face.vFaceMaxs - face.vFaceMins;
    glm::vec2 baseGridSizeFloat = faceSize / face.flPatchSize;
    glm::ivec2 baseGridSize;
    baseGridSize.x = texFloatToInt(baseGridSizeFloat.x);
    baseGridSize.y = texFloatToInt(baseGridSizeFloat.y);

    // Allow small patches for small faces
    if (baseGridSizeFloat.x < 1 || baseGridSizeFloat.y < 1) {
        flMinPatchSize = MIN_PATCH_SIZE_FOR_SMALL_FACES;
    }

    // Create an empty first patch. It will be removed at the end
    AFW_ASSERT(!m_pPatches);
    MiniPatch emptyPatch;
    MiniPatch *pPrevPatch = &emptyPatch;

    PatchIndex patchCount = 0;

    for (int y = 0; y < baseGridSize.y; y++) {
        for (int x = 0; x < baseGridSize.x; x++) {
            MiniPatch patch;
            patch.flSize = face.flPatchSize;
            patch.vFaceOrigin = m_pFace->vFaceMins + glm::vec2(x, y) / baseGridSizeFloat * faceSize;

            MiniPatch *subdNewFirstPatch = nullptr;
            MiniPatch *subdNewLastPatch = nullptr;
            bool keepPatch = true;
            patchCount += subdividePatch(&patch, subdNewFirstPatch, subdNewLastPatch, keepPatch,
                                         flMinPatchSize);

            if (keepPatch) {
                AFW_ASSERT(!subdNewFirstPatch && !subdNewLastPatch);
                MiniPatch *newPatch = new MiniPatch(patch);
                pPrevPatch->pNext = newPatch;
                pPrevPatch = newPatch;
                patchCount++;
            } else {
                if (subdNewFirstPatch) {
                    AFW_ASSERT(subdNewLastPatch);
                    pPrevPatch->pNext = subdNewFirstPatch;
                    pPrevPatch = subdNewLastPatch;
                }
            }
        }
    }

    globalPatchCount += patchCount;
    m_pPatches = emptyPatch.pNext;
}

rad::PatchIndex rad::PatchTree::copyPatchesToTheList(PatchIndex offset, appfw::SHA256 &hash) {
    PatchIndex count = 0;
    MiniPatch *p = m_pPatches;

    while (p) {
        PatchRef patch(m_pRadSim->m_Patches, offset);

        glm::vec2 faceOrg = p->vFaceOrigin + glm::vec2(p->flSize / 2.0f);
        patch.getSize() = p->flSize;
        patch.getFaceOrigin() = faceOrg;
        patch.getOrigin() = m_pFace->faceToWorld(faceOrg);
        patch.getNormal() = m_pFace->vNormal;
        patch.getPlane() = m_pFace->pPlane;

        // Add origin and normal to hash
        hash.update(reinterpret_cast<uint8_t *>(&patch.getOrigin()), sizeof(glm::vec3));
        hash.update(reinterpret_cast<uint8_t *>(&patch.getNormal()), sizeof(glm::vec3));

        count++;
        offset++;

        MiniPatch *del = p;
        p = p->pNext;
        delete del;
    }

    m_pPatches = nullptr;
    return count;
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
                                 float &weightSum) {
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

    for (PatchIndex i = 0; i < count; i++) {
        PatchRef patch(m_pRadSim->m_Patches, m_pFace->iFirstPatch + i);

        glm::vec2 d = patch.getFaceOrigin() - pos;

        if (std::abs(d.x) <= radius && std::abs(d.y) <= radius) {
            float weight = lightmapFilter(d.x * filterk) * lightmapFilter(d.y * filterk);
            out += weight * patch.getFinalColor();
            weightSum += weight;
        }
    }
#endif
}

rad::PatchIndex rad::PatchTree::subdividePatch(MiniPatch *patch, MiniPatch *&newFirstPatch,
                                               MiniPatch *&newLastPatch, bool &keepPatch,
                                               float flMinPatchSize) noexcept {
    AFW_ASSERT(!patch->pNext);
    PatchPos pos = checkPatchPos(patch);

    if (pos == PatchPos::Inside) {
        // Keep the patch as is
        keepPatch = true;
        return 0;
    } else if (pos == PatchPos::Outside) {
        // Remove it
        keepPatch = false;
        return 0;
    }

    float divSize = patch->flSize / 2.0f;
    if (divSize < flMinPatchSize) {
        // Patch is too small for subdivision, keep only if the center is in face
        keepPatch = isInFace(patch->vFaceOrigin + glm::vec2(patch->flSize / 2.0f));
        return 0;
    }

    // Patch needs to be divided into 4 smaller patches
    keepPatch = false;
    newFirstPatch = nullptr;
    newLastPatch = nullptr;
    MiniPatch newPatches[4];
    bool keepNewPatches[4];
    PatchIndex createdPatches = 0;

    newPatches[0].flSize = divSize;
    newPatches[0].vFaceOrigin = patch->vFaceOrigin;

    newPatches[1].flSize = divSize;
    newPatches[1].vFaceOrigin = patch->vFaceOrigin;
    newPatches[1].vFaceOrigin.x += divSize;

    newPatches[2].flSize = divSize;
    newPatches[2].vFaceOrigin = patch->vFaceOrigin;
    newPatches[2].vFaceOrigin.y += divSize;

    newPatches[3].flSize = divSize;
    newPatches[3].vFaceOrigin = patch->vFaceOrigin;
    newPatches[3].vFaceOrigin.x += divSize;
    newPatches[3].vFaceOrigin.y += divSize;

    for (size_t i = 0; i < 4; i++) {
        MiniPatch *subdNewFirstPatch = nullptr;
        MiniPatch *subdNewLastPatch = nullptr;
        createdPatches += subdividePatch(&newPatches[i], subdNewFirstPatch, subdNewLastPatch,
                                         keepNewPatches[i], flMinPatchSize);

        if (keepNewPatches[i]) {
            AFW_ASSERT(!subdNewFirstPatch && !subdNewLastPatch);
            MiniPatch *newPatch = new MiniPatch(newPatches[i]);
            createdPatches++;

            if (!newFirstPatch) {
                AFW_ASSERT(!newLastPatch);
                newFirstPatch = newPatch;
            } else {
                AFW_ASSERT(newLastPatch);
                newLastPatch->pNext = newPatch;
            }

            newLastPatch = newPatch;
        } else {
            if (subdNewFirstPatch) {
                AFW_ASSERT(subdNewLastPatch);

                if (!newFirstPatch) {
                    AFW_ASSERT(!newLastPatch);
                    newFirstPatch = subdNewFirstPatch;
                } else {
                    AFW_ASSERT(newLastPatch);
                    newLastPatch->pNext = subdNewFirstPatch;
                }

                newLastPatch = subdNewLastPatch;
            }
        }
    }

    return createdPatches;
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

rad::PatchTree::PatchPos rad::PatchTree::checkPatchPos(MiniPatch *patch) noexcept {
    glm::vec2 corners[4];
    corners[0] = patch->vFaceOrigin;

    corners[1] = patch->vFaceOrigin;
    corners[1].x += patch->flSize;

    corners[2] = patch->vFaceOrigin;
    corners[2].y += patch->flSize;

    corners[3] = patch->vFaceOrigin;
    corners[3].x += patch->flSize;
    corners[3].y += patch->flSize;

    int count = 0;
    
    for (int i = 0; i < 4; i++) {
        count += (int)isInFace(corners[i]);
    }

    if (count == 4) {
        return PatchPos::Inside;
    } else if (count == 0) {
        // See if the face is inside the patch
        size_t verts = m_pFace->vertices.size();
        for (size_t i = 0; i < verts; i++) {
            if (pointIntersectsWithRect(m_pFace->vertices[i].vFacePos, patch->vFaceOrigin,
                                        patch->flSize)) {
                return PatchPos::PartiallyInside;
            }
        }

        return PatchPos::Outside;
    } else {
        return PatchPos::PartiallyInside;
    }
}

bool rad::PatchTree::isInFace(glm::vec2 point2d) noexcept {
    // TODO: 3D when it can be done in 2D
    // Doesn't seem slow though...
    size_t count = m_pFace->vertices.size();
    glm::vec3 point = m_pFace->faceToWorld(point2d);

    for (size_t i = 0; i < count; i++) {
        glm::vec3 a = m_pFace->faceToWorld(m_pFace->vertices[i].vFacePos);
        glm::vec3 b = m_pFace->faceToWorld(m_pFace->vertices[(i + 1) % count].vFacePos);
        glm::vec3 edge = b - a;
        glm::vec3 p = point - a;

        if (glm::dot(m_pFace->vNormal, glm::cross(edge, p)) > 0) {
            return false;
        }
    }

    return true;
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
