#include <appfw/binary_file.h>
#include <rad/vismat.h>
#include <rad/rad_sim.h>
#include "plat.h"

rad::VisMat::VisMat(RadSim *pRadSim) { m_pRadSim = pRadSim; }

bool rad::VisMat::isLoaded() { return m_bIsLoaded; }

bool rad::VisMat::isValid() { return m_bIsLoaded && m_PatchHash == m_pRadSim->getPatchHash(); }

size_t rad::VisMat::getCurrentSize() { return m_Data.size(); }

size_t rad::VisMat::getValidSize() {
    size_t patches = m_pRadSim->m_Patches.size();
    return ((patches + 1) * (((patches + 1) + 15) / 16)); // TODO: Magic
}

void rad::VisMat::loadFromFile(const fs::path &path) {
    m_bIsLoaded = false;
    m_PatchHash = {};
    
    appfw::BinaryReader file(path);

    // Read magic
    char magic[sizeof(VISMAT_MAGIC)];
    file.readBytes(magic, sizeof(VISMAT_MAGIC));
    if (memcmp(magic, VISMAT_MAGIC, sizeof(VISMAT_MAGIC))) {
        logInfo("VisMat discarded: unsupported format.");
        return;
    }

    // Read pointer size
    uint8_t ptrSize;
    file.read(ptrSize);
    if (ptrSize != sizeof(void *)) {
        logInfo("VisMat discarded: different pointer size.");
        return;
    }

    // Read patch hash
    appfw::SHA256::Digest patchHash;
    file.readArray(appfw::span(patchHash));

    if (patchHash != m_pRadSim->m_PatchHash) {
        logInfo("VisMat discarded: different patch hash.");
        return;
    }

    // Read size
    size_t size;
    size_t validSize = getValidSize();
    file.read(size);

    if (size != validSize) {
        logInfo("VisMat discarded: invalid size ({} instead of {}).", size, validSize);
        return;
    }

    // Resize
    if (m_Data.size() != size) {
        std::vector<uint8_t>().swap(m_Data);
        m_Data.resize(size);
    }

    // Read data
    file.readArray(appfw::span(m_Data));
    m_bIsLoaded = true;
    m_PatchHash = patchHash;

    logInfo("Reusing previous vismat.");
}

void rad::VisMat::saveToFile(const fs::path &path) {
    if (!m_bIsLoaded) {
        throw std::logic_error("VisMat::saveToFile: vismat not loaded");
    }

    appfw::BinaryWriter file(path);
    file.writeBytes(VISMAT_MAGIC, sizeof(VISMAT_MAGIC));
    file.write<uint8_t>(sizeof(void *));
    file.writeArray(appfw::span(m_PatchHash));
    file.write(m_Data.size());
    file.writeArray(appfw::span(m_Data));
}

void rad::VisMat::buildVisMat() {
    m_pRadSim->updateProgress(0);
    m_bIsLoaded = false;
    std::vector<uint8_t>().swap(m_Data); // Free allocated memory
    m_PatchHash = m_pRadSim->getPatchHash();

    size_t size = getValidSize();
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

bool rad::VisMat::checkVisBit(PatchIndex _p1, PatchIndex _p2) {
    if (_p1 > _p2) {
        std::swap(_p1, _p2);
    }

    size_t p1 = _p1;
    size_t p2 = _p2;
    size_t patches = m_pRadSim->m_Patches.size();
    size_t bitpos = p1 * patches - (p1 * (p1 + 1)) / 2 + p2;

    if (m_Data[bitpos >> 3] & (1 << (bitpos & 7))) {
        return true;
    } else {
        return false;
    }
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

                size_t bitpos = patchnum * m_pRadSim->m_Patches.size() - (patchnum * (patchnum + 1)) / 2;

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
            plat::atomicSetBit(&m_Data[bitset >> 3], bitset & 7);
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
