#include "patch_divider.h"

rad::PatchDivider::~PatchDivider() {
    while (m_pPatches) {
        MiniPatch *p = m_pPatches;
        m_pPatches = p->pNext;
        delete p;
    }
}

rad::PatchIndex rad::PatchDivider::createPatches(RadSimImpl *pRadSim, Face &face,
                                                 PatchIndex offset) {
    // Calculate base size of patch grid (wide x tall patches of flPatchSize)
    glm::vec2 faceSize = face.vFaceMaxs - face.vFaceMins;
    glm::vec2 baseGridSizeFloat = faceSize / face.flPatchSize;
    glm::ivec2 baseGridSize;
    baseGridSize.x = texFloatToInt(baseGridSizeFloat.x);
    baseGridSize.y = texFloatToInt(baseGridSizeFloat.y);

    // Allow small patches for small faces
    float flMinPatchSize = pRadSim->getMinPatchSize();
    if (faceSize.x <= SMALL_FACE_SIZE || faceSize.y <= SMALL_FACE_SIZE) {
        flMinPatchSize = MIN_PATCH_SIZE_FOR_SMALL_FACES;
    }

    // Create an empty first patch
    MiniPatch emptyPatch;
    MiniPatch *pPrevPatch = &emptyPatch;
    PatchIndex patchCount = 0;

    for (int y = 0; y < baseGridSize.y; y++) {
        for (int x = 0; x < baseGridSize.x; x++) {
            MiniPatch patch;
            patch.pFace = &face;
            patch.flSize = face.flPatchSize;
            patch.vFaceOrigin = face.vFaceMins + glm::vec2(x, y) / baseGridSizeFloat * faceSize;

            MiniPatch *subdNewFirstPatch = nullptr;
            MiniPatch *subdNewLastPatch = nullptr;
            bool keepPatch = true;
            patchCount += subdividePatch(face, &patch, subdNewFirstPatch, subdNewLastPatch,
                                         keepPatch, flMinPatchSize);

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

    // Save patch indicies in the face
    face.iFirstPatch = offset;
    face.iNumPatches = patchCount;

    if (patchCount > 0) {
        if (m_pPatches) {
            AFW_ASSERT(m_pLastPatch);
            AFW_ASSERT(!m_pLastPatch->pNext);
            AFW_ASSERT(!pPrevPatch->pNext);
            m_pLastPatch->pNext = emptyPatch.pNext;
        } else {
            m_pPatches = emptyPatch.pNext;
        }

        m_pLastPatch = pPrevPatch;
    }

    return patchCount;
}

rad::PatchIndex rad::PatchDivider::transferPatches(RadSimImpl *pRadSim, appfw::SHA256 &hash) {
    PatchIndex count = 0;
    MiniPatch *p = m_pPatches;

    while (p) {
        Face &face = *p->pFace;
        PatchRef patch(pRadSim->m_Patches, count);

        glm::vec2 faceOrg = p->vFaceOrigin + glm::vec2(p->flSize / 2.0f);
        patch.getSize() = p->flSize;
        patch.getFaceOrigin() = faceOrg;
        patch.getOrigin() = face.faceToWorld(faceOrg);
        patch.getRealOrigin() = patch.getOrigin() + face.vBrushOrigin; 
        patch.getNormal() = face.vNormal;
        patch.getPlane() = face.pPlane;
        patch.getReflectivity() = glm::vec3(face.flBaseReflectivity);

        // Add origin and normal to hash
        hash.update(reinterpret_cast<uint8_t *>(&patch.getRealOrigin()), sizeof(glm::vec3));
        hash.update(reinterpret_cast<uint8_t *>(&patch.getNormal()), sizeof(glm::vec3));

        count++;

        MiniPatch *del = p;
        p = p->pNext;
        delete del;
    }

    m_pPatches = nullptr;
    return count;
}

rad::PatchIndex rad::PatchDivider::subdividePatch(Face &face, MiniPatch *patch,
                                                  MiniPatch *&newFirstPatch,
                                                  MiniPatch *&newLastPatch, bool &keepPatch,
                                                  float flMinPatchSize) {
    AFW_ASSERT(!patch->pNext);
    PatchPos pos = checkPatchPos(face, patch);

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
        keepPatch = isInFace(face, patch->vFaceOrigin + glm::vec2(patch->flSize / 2.0f));
        return 0;
    }

    // Patch needs to be divided into 4 smaller patches
    keepPatch = false;
    newFirstPatch = nullptr;
    newLastPatch = nullptr;
    MiniPatch newPatches[4];
    bool keepNewPatches[4];
    PatchIndex createdPatches = 0;

    newPatches[0].vFaceOrigin = patch->vFaceOrigin;

    newPatches[1].vFaceOrigin = patch->vFaceOrigin;
    newPatches[1].vFaceOrigin.x += divSize;

    newPatches[2].vFaceOrigin = patch->vFaceOrigin;
    newPatches[2].vFaceOrigin.y += divSize;

    newPatches[3].vFaceOrigin = patch->vFaceOrigin;
    newPatches[3].vFaceOrigin.x += divSize;
    newPatches[3].vFaceOrigin.y += divSize;

    for (size_t i = 0; i < 4; i++) {
        newPatches[i].flSize = divSize;
        newPatches[i].pFace = &face;

        MiniPatch *subdNewFirstPatch = nullptr;
        MiniPatch *subdNewLastPatch = nullptr;
        createdPatches += subdividePatch(face, &newPatches[i], subdNewFirstPatch, subdNewLastPatch,
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

rad::PatchDivider::PatchPos rad::PatchDivider::checkPatchPos(Face &face,
                                                             MiniPatch *patch) noexcept {
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
        count += (int)isInFace(face, corners[i]);
    }

    if (count == 4) {
        return PatchPos::Inside;
    } else if (count == 0) {
        // See if the face is inside the patch
        size_t verts = face.vertices.size();
        for (size_t i = 0; i < verts; i++) {
            if (pointInWithPatch(face.vertices[i].vFacePos, patch)) {
                return PatchPos::PartiallyInside;
            }
        }

        return PatchPos::Outside;
    } else {
        return PatchPos::PartiallyInside;
    }
}

bool rad::PatchDivider::isInFace(Face &face, glm::vec2 point2d) noexcept {
    size_t count = face.vertices.size();
    glm::vec3 point = face.faceToWorld(point2d);

    for (size_t i = 0; i < count; i++) {
        glm::vec3 a = face.faceToWorld(face.vertices[i].vFacePos);
        glm::vec3 b = face.faceToWorld(face.vertices[(i + 1) % count].vFacePos);
        glm::vec3 edge = b - a;
        glm::vec3 p = point - a;

        if (glm::dot(face.vNormal, glm::cross(edge, p)) > 0) {
            return false;
        }
    }

    return true;
}

bool rad::PatchDivider::pointInWithPatch(glm::vec2 point, MiniPatch *patch) {
    return (point.x >= patch->vFaceOrigin.x && point.x <= patch->vFaceOrigin.x + patch->flSize) &&
           (point.y >= patch->vFaceOrigin.y && point.y <= patch->vFaceOrigin.y + patch->flSize);
}
