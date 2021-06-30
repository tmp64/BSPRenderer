#include <rad/patch_tree.h>
#include <rad/rad_sim.h>
#include <rad/patch_list.h>

// #define DISABLE_TREE

void rad::PatchTree::createPatches(RadSim *pRadSim, Face &face,
                                   std::atomic<PatchIndex> &globalPatchCount,
                                   float flMinPatchSize) {
    m_pRadSim = pRadSim;
    m_pFace = &face;

    // Calculate base size of patch grid (wide x tall patches of flPatchSize)
    glm::vec2 baseGridSizeFloat = face.planeMaxBounds / face.flPatchSize;
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
            patch.vFaceOrigin = glm::vec2(x, y) / baseGridSizeFloat * face.planeMaxBounds;

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
        patch.getOrigin() =
            m_pFace->vPlaneOrigin + m_pFace->vI * faceOrg.x + m_pFace->vJ * faceOrg.y;
        patch.getNormal() = m_pFace->vNormal;
        patch.getPlane() = m_pFace->pPlane;

        // Add origin and plane to hash
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
    PatchIndex patchCount = m_pFace->iNumPatches;
    float minSize = m_pFace->flPatchSize * 2.0f;

    // Set up root node
    float size = std::max(m_pFace->planeMaxBounds.x, m_pFace->planeMaxBounds.y);
    m_RootNode.flSize = size;
    m_RootNode.vOrigin = m_pFace->planeCenterPoint;

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
}

bool rad::PatchTree::sampleLight(const glm::vec2 &pos, float radius, glm::vec3 &out) {
#ifndef DISABLE_TREE
    glm::vec2 corners[4];
    getCorners(pos, radius * 2.0f, corners);
    out = glm::vec3(0, 0, 0);
    float weightSum = 0;

    recursiveSample(&m_RootNode, pos, radius, out, corners, weightSum);

    if (weightSum == 0) {
        return false;
    }

    out /= weightSum;
    return true;
#else
    PatchIndex count = m_pFace->iNumPatches;

    out = glm::vec3(0, 0, 0);
    float weightSum = 0;

    for (PatchIndex i = 0; i < count; i++) {
        PatchRef patch(m_pRadSim->m_Patches, m_pFace->iFirstPatch + i);

        glm::vec2 d = patch.getFaceOrigin() - pos;

        if (std::abs(d.x) < radius && std::abs(d.y) < radius) {
            float weight = glm::length(d);
            out += weight * patch.getFinalColor();
            weightSum += weight;
        }
    }

    if (weightSum == 0) {
        return false;
    }

    out /= weightSum;
    return true;
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

        if (needToCheck || true) {
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
            if (pointIntersectsWithRect(m_pFace->vertices[i].vPlanePos, patch->vFaceOrigin,
                                        patch->flSize)) {
                return PatchPos::PartiallyInside;
            }
        }

        return PatchPos::Outside;
    } else {
        return PatchPos::PartiallyInside;
    }
}

bool rad::PatchTree::isInFace(glm::vec2 point) noexcept {
    size_t count = m_pFace->vertices.size();

    for (size_t i = 0; i < count; i++) {
        glm::vec2 a = m_pFace->vertices[i].vPlanePos;
        glm::vec2 b = m_pFace->vertices[(i + 1) % count].vPlanePos;
        glm::vec2 seg = b - a;
        glm::vec2 p = point - a;
        float cossign = seg.x * p.y - seg.y * p.x;

        if (cossign > 0) {
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
