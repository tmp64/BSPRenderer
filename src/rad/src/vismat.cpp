#include <iostream>
#include <appfw/binary_file.h>
#include <appfw/timer.h>
#include <rad/vismat.h>
#include <rad/rad_sim.h>
#include "plat.h"

//#define VISMAT_DEBUG 1

rad::VisMat::VisMat(RadSim *pRadSim) { m_pRadSim = pRadSim; }

bool rad::VisMat::isLoaded() { return m_bIsLoaded; }

bool rad::VisMat::isValid() { return m_bIsLoaded && m_PatchHash == m_pRadSim->getPatchHash(); }

size_t rad::VisMat::getCurrentSize() { return m_Data.size(); }

void rad::VisMat::buildVisMat() {
    m_pRadSim->updateProgress(0);
    unloadVisMat();
    m_PatchHash = m_pRadSim->getPatchHash();

    size_t size = calculateOffsets(m_Offsets);
    logInfo("Visibility matrix: {:5.1f} MiB", size / (1024 * 1024.0));
    m_Data.resize(size);

    // Calc vismat in multiple threads
    // Skip 0-th leaf as it's the solid leaf
    m_pRadSim->m_Dispatcher.setWorkCount(m_pRadSim->m_pLevel->getLeaves().size(), 1);
    m_pRadSim->m_ThreadPool.run([this](auto &ti) { buildVisLeaves(ti); }, 0);

    size_t leafCount = m_pRadSim->m_pLevel->getLeaves().size();

    while (!m_pRadSim->m_ThreadPool.isFinished()) {
        double done = (double)m_pRadSim->m_ThreadPool.getStatus() / leafCount;
        m_pRadSim->updateProgress(done);
        std::this_thread::sleep_for(m_pRadSim->m_ThreadPool.POLL_DELAY);
    }

    if (m_pRadSim->m_ThreadPool.getStatus() != leafCount - 1) {
        logError("Threading failure!");
        abort();
    }

    m_pRadSim->updateProgress(1);

    m_bIsLoaded = true;
}

bool rad::VisMat::checkVisBit(PatchIndex p1, PatchIndex p2) {
    if (p1 > p2) {
        std::swap(p1, p2);
    } else if (p1 == p2) {
        return false;
    }

    // p1 < p2 is always true
    size_t bitpos = getPatchBitPos(p1, p2);

#ifdef VISMAT_DEBUG
    if ((bitpos >> 3) > m_Data.size()) {
        logFatal("p1: {}", p1);
        logFatal("p2: {}", p2);
        logFatal("ol: {}", m_Offsets[p1]);
        logFatal("or: {}", m_Offsets[p1 + 1]);
        logFatal("d:  {}", (p2 - p1 - 1));
        abort();
    }
#endif

    if (m_Data[bitpos >> 3] & (1 << (bitpos & 7))) {
        return true;
    } else {
        return false;
    }
}

void rad::VisMat::unloadVisMat() {
    m_bIsLoaded = false;
    std::vector<size_t>().swap(m_Offsets);
    std::vector<uint8_t>().swap(m_Data);
}

size_t rad::VisMat::calculateOffsets(std::vector<size_t> &offsets) {
    logInfo("Calculating vismat offsets...");
    appfw::Timer timer;
    timer.start();

    size_t totalSize = 0;
    PatchIndex patchCount = m_pRadSim->m_Patches.size();

    offsets.resize(patchCount);

    for (PatchIndex i = 0; i < patchCount; i++) {
        offsets[i] = totalSize;

        size_t rowLen = (size_t)patchCount - i - 1;
        
        // Align to ROW_ALIGNMENT bytes
        size_t align = rowLen / ROW_ALIGNMENT_BITS * ROW_ALIGNMENT_BITS; // rowLen truncated to be alined
        rowLen += ROW_ALIGNMENT_BITS + align - rowLen; // adds some bits to make it divisable by ROW_ALIGNMENT_BITS

#ifdef VISMAT_DEBUG
        if (rowLen % ROW_ALIGNMENT_BITS != 0) {
            logFatal("Vis row {} is unaligned {}", i, rowLen);
            abort();
        }

        if (rowLen < patchCount - i - 1) {
            abort();
        }
#endif

        totalSize += rowLen;
    }


    timer.stop();
    logInfo("Calculate vismat offsets: {:.3f} s", timer.elapsedSeconds());

    if (totalSize % ROW_ALIGNMENT_BITS != 0) {
        logFatal("Vismat is unaligned {}", totalSize);
        abort();
    }

    return totalSize / 8;
}

void rad::VisMat::buildVisLeaves(appfw::ThreadPool::ThreadInfo &ti) {
    int lface, facenum;
    uint8_t pvs[(bsp::MAX_MAP_LEAFS + 7) / 8];

    auto &leaves = m_pRadSim->m_pLevel->getLeaves();
    auto &visdata = m_pRadSim->m_pLevel->getVisData();
    auto &marksurfaces = m_pRadSim->m_pLevel->getMarkSurfaces();
    auto &dispatcher = m_pRadSim->m_Dispatcher;

    std::vector<uint8_t> face_tested(bsp::MAX_MAP_FACES);

    for (size_t i = dispatcher.getWork(); i != dispatcher.WORK_DONE; i = dispatcher.getWork(), ti.iWorkDone++) {
        //
        // build a minimal BSP tree that only
        // covers areas relevent to the PVS
        //
        const bsp::BSPLeaf *srcleaf = &leaves[i];

        if (srcleaf->nVisOffset == -1) {
            // Skip leaves wothout vis data
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
            Face &face = m_pRadSim->m_Faces[facenum];

            for (PatchIndex patchIdx = 0; patchIdx < face.iNumPatches; patchIdx++) {
                PatchIndex patchnum = face.iFirstPatch + patchIdx;

                // FIXME: Doesn't work properly. Discards way too many patches.
                // Patch &patch = g_Patches[patchnum];
                // const bsp::BSPLeaf *leaf = pointInLeaf(patch.vOrigin);
                // if (leaf != srcleaf) {
                //     continue;
                // }

                size_t bitpos = getRowOffset(patchnum);

                // build to all other world leafs
                buildVisRow(patchnum, pvs, bitpos, face_tested);

                // build to bmodel faces (to brush entities)
                /*if (nummodels < 2)
                    continue;
                for (int facenum2 = dmodels[1].firstface; facenum2 < numfaces; facenum2++)
                    TestPatchToFace(patchnum, facenum2, head, bitpos);*/
            }
        }
    }
}

void rad::VisMat::buildVisRow(PatchIndex patchnum, uint8_t *pvs, size_t bitpos, std::vector<uint8_t> &face_tested) {
    std::fill(face_tested.begin(), face_tested.end(), (uint8_t)0);

    auto &leaves = m_pRadSim->m_pLevel->getLeaves();
    auto &marksurfaces = m_pRadSim->m_pLevel->getMarkSurfaces();

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

            if (m_pRadSim->m_Faces[l].iFlags & FACE_SKY) {
                continue;
            }

            testPatchToFace(patchnum, l, bitpos);
        }
    }
}

void rad::VisMat::testPatchToFace(PatchIndex patchnum, int facenum, size_t bitpos) {
    PatchRef patch(m_pRadSim->m_Patches, patchnum);

    for (PatchIndex i = 0; i < m_pRadSim->m_Faces[facenum].iNumPatches; i++) {
        PatchIndex m = m_pRadSim->m_Faces[facenum].iFirstPatch + i;
        PatchRef patch2(m_pRadSim->m_Patches, m);

        // check vis between patch and patch2
        // if bit has not already been set
        // and p2 is visible from p1
        if (m > patchnum &&
            m_pRadSim->m_pLevel->traceLine(patch.getOrigin(), patch2.getOrigin()) == bsp::CONTENTS_EMPTY) {

            // patchnum can see patch m
            size_t bitset = bitpos + m;

#ifdef VISMAT_DEBUG
            if ((bitset >> 3) >= m_Data.size()) {
                std::cerr << fmt::format("testPatchToFace: bitset overflow\n");
                std::cerr << fmt::format("Size:   {}\n", m_Data.size());
                std::cerr << fmt::format("Set>>3: {}\n", bitset >> 3);
                std::cerr << fmt::format("Bitpos: {}\n", bitpos);
                std::cerr << fmt::format("m:      {}\n", m);
                std::cerr << fmt::format("Bitset: {}\n", bitset);
                abort();
            }
#endif
            // Each row is aligned to byte boundary
            // Data race is not possible, atomic bit set not needed.
            m_Data[bitset >> 3] |= 1 << (bitset & 7);
            //plat::atomicSetBit(&m_Data[bitset >> 3], bitset & 7);
        }
    }
}

void rad::VisMat::decompressVis(const uint8_t *in, uint8_t *decompressed) {
    size_t row = (m_pRadSim->m_pLevel->getLeaves().size() + 7) >> 3;
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
