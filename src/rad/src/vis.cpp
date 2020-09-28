#include <appfw/services.h>
#include <appfw/timer.h>
#include "main.h"
#include "vis.h"
#include "bsp_tree.h"
#include <appfw/binary_file.h>

// Decreases memory usage in half
#define HALFBIT

static std::vector<uint8_t> s_VisMatrix;

static appfw::BinaryWriter s_BinFile;

void buildVisMatrix() {
    logInfo("Building visibility matrix...");
    appfw::Timer timer;
    s_BinFile.open("test.dat");
    timer.start();

#ifdef HALFBIT
    size_t c = ((g_Patches.size() + 1) * (((g_Patches.size() + 1) + 15) / 16));
#else
    size_t c = g_Patches.size() * ((g_Patches.size() + 7) / 8);
#endif

    logInfo("Visibility matrix: {:5.1f} MiB", c / (1024 * 1024.0));
    s_VisMatrix.resize(c);
    
    // TODO: Multithreading?
    buildVisLeaves();

    timer.stop();
    logInfo("        ... {:.3} s", timer.elapsedSeconds());
}

void buildVisLeaves() {
    int lface, facenum;
    uint8_t pvs[(bsp::MAX_MAP_LEAFS + 7) / 8];

    auto &leaves = g_Level.getLeaves();
    auto &visdata = g_Level.getVisData();
    auto &marksurfaces = g_Level.getMarkSurfaces();

    for (size_t i = 1; i < leaves.size(); i++) {
        //
        // build a minimal BSP tree that only
        // covers areas relevent to the PVS
        //
        const bsp::BSPLeaf *srcleaf = &leaves[i];

        if (srcleaf->nVisOffset == -1) {
            //logError("Error: map doesn't have visibility data.");
            //exit(1);
            continue;
        }

        memset(pvs, 0, sizeof(pvs));
        decompressVis(&visdata[srcleaf->nVisOffset], pvs);

        //
        // go through all the faces inside the
        // leaf, and process the patches that
        // actually have origins inside
        //
        for (lface = 0; lface < srcleaf->nMarkSurfaces; lface++) {
            facenum = marksurfaces[srcleaf->iFirstMarkSurface + lface];
            Face &face = g_Faces[facenum];

            for (int patchIdx = 0; patchIdx < face.iNumPatches; patchIdx++) {
                size_t patchnum = face.iFirstPatch + patchIdx;
                Patch &patch = g_Patches[patchnum];

                // FIXME: Doesn't work properly. Discards way too many patches.
                //const bsp::BSPLeaf *leaf = pointInLeaf(patch.vOrigin);
                //if (leaf != srcleaf)
                //    continue;

#ifdef HALFBIT
                size_t bitpos = patchnum * g_Patches.size() - (patchnum * (patchnum + 1)) / 2;
#else
                size_t bitpos = patchnum * g_Patches.size();
#endif

                // build to all other world leafs
                buildVisRow(patchnum, pvs, bitpos);

                // build to bmodel faces (to brush entities)
                /*if (nummodels < 2)
                    continue;
                for (int facenum2 = dmodels[1].firstface; facenum2 < numfaces; facenum2++)
                    TestPatchToFace(patchnum, facenum2, head, bitpos);*/
            }
        }
    }
}

/*
==============
BuildVisRow

Calc vis bits from a single patch
==============
*/
void buildVisRow(size_t patchnum, uint8_t *pvs, size_t bitpos) {
    std::vector<uint8_t> face_tested(bsp::MAX_MAP_FACES);

    auto &leaves = g_Level.getLeaves();
    auto &marksurfaces = g_Level.getMarkSurfaces();

    // leaf 0 is the solid leaf (skipped)
    for (int j = 1; j < leaves.size(); j++) {
        const bsp::BSPLeaf &leaf = leaves[j];

        if (!(pvs[(j - 1) >> 3] & (1 << ((j - 1) & 7))))
            continue; // not in pvs

        for (int k = 0; k < leaf.nMarkSurfaces; k++) {
            int l = marksurfaces[leaf.iFirstMarkSurface + k];

            // faces can be marksurfed by multiple leaves, but
            // don't bother testing again
            if (face_tested[l])
                continue;
            face_tested[l] = 1;

            if (g_Faces[l].iFlags & FACE_SKY) {
                continue;
            }

            testPatchToFace(patchnum, l, bitpos);
        }
    }
}

/*
==============
PatchPlaneDist

Fixes up patch planes for brush models with an origin brush
==============
*/
/*float PatchPlaneDist(Patch &patch) {
    return patch.pPlane->fDist + glm::dot(face_offset[patch->faceNumber], patch->normal);
}*/

/*
==============
TestPatchToFace

Sets vis bits for all patches in the face
==============
*/
void testPatchToFace(size_t patchnum, int facenum, size_t bitpos) {
    Patch &patch = g_Patches[patchnum];
    Patch &firstFacePatch = g_Patches[g_Faces[facenum].iFirstPatch];

    // if emitter is behind that face plane, skip all patches

    if (/* glm::dot(patch.vOrigin, firstFacePatch.vNormal) > firstFacePatch.pPlane->fDist */ true) {
        // we need to do a real test
        for (size_t i = 0; i < g_Faces[facenum].iNumPatches; i++) {
            size_t m = g_Faces[facenum].iFirstPatch + i;
            Patch &patch2 = g_Patches[m];

            // check vis between patch and patch2
            // if bit has not already been set
            //  && v2 is not behind light plane
            //  && v2 is visible from v1
            if (/* m > patchnum && */ /*glm::dot(patch2.vOrigin, patch.vNormal) > patch.pPlane->fDist &&*/
                !g_BSPTree.traceLine(patch.vOrigin + patch.vNormal * 0.1f, patch2.vOrigin + patch2.vNormal * 0.1f)) {

                // patchnum can see patch m
                //size_t bitset = bitpos + m;

#ifdef HALFBIT
                size_t bitset1 = patchnum * g_Patches.size() - (patchnum * (patchnum + 1)) / 2 + m;
                size_t bitset2 = m * g_Patches.size() - (m * (m + 1)) / 2 + patchnum;
#else
                size_t bitset1 = patchnum * g_Patches.size() + m;
                size_t bitset2 = m * g_Patches.size() + patchnum;
#endif

                s_VisMatrix[bitset1 >> 3] |= 1 << (bitset1 & 7);
                s_VisMatrix[bitset2 >> 3] |= 1 << (bitset2 & 7);
            }
        }
    }
}

void decompressVis(const uint8_t *in, uint8_t *decompressed) {
    size_t row = (g_Level.getLeaves().size() + 7) >> 3;
    uint8_t *out = decompressed;

    do {
        if (*in) {
            *out++ = *in++;
            continue;
        }

        int c = in[1];
        in += 2;
        while (c) {
            *out++ = 0;
            c--;
        }
    } while ((size_t)(out - decompressed) < row);
}

const bsp::BSPLeaf *pointInLeaf(glm::vec3 point) {
    /*int nodenum;
    float dist;
    const bsp::BSPNode *node;
    const bsp::BSPPlane *plane;

    nodenum = 0;
    while (nodenum >= 0) {
        node = &g_Level.getNodes()[nodenum];
        plane = &g_Level.getPlanes()[node->iPlane];
        dist = glm::dot(point, plane->vNormal) - plane->fDist;
        if (dist > 0)
            nodenum = node->iChildren[0];
        else
            nodenum = node->iChildren[1];
    }

    return &g_Level.getLeaves()[-nodenum - 1];*/


    int iNode = 0;

    for (;;) {
        if (iNode < 0) {
            return &g_Level.getLeaves()[-iNode - 1];
        }

        const bsp::BSPNode *pNode = &g_Level.getNodes()[iNode];
        const bsp::BSPPlane &plane = g_Level.getPlanes()[pNode->iPlane];
        float d = glm::dot(point, plane.vNormal) - plane.fDist;

        if (d > 0) {
            iNode = pNode->iChildren[0];
        } else {
            iNode = pNode->iChildren[1];
        }
    }
}

/*
==============
CheckVisBit
==============
*/
bool checkVisBit(size_t p1, size_t p2) {

    if (p1 > p2) {
        std::swap(p1, p2);
    }

#ifdef HALFBIT
    size_t bitpos = p1 * g_Patches.size() - (p1 * (p1 + 1)) / 2 + p2;
#else
    size_t bitpos = p1 * g_Patches.size() + p2;
#endif

    if (s_VisMatrix[bitpos >> 3] & (1 << (bitpos & 7)))
        return true;
    return false;
}
