#include <vector>
#include <forward_list>
#include <appfw/timer.h>
#include <appfw/platform.h>
#include <appfw/binary_file.h>
#include <rad/sparse_vismat.h>
#include <rad/rad_sim.h>

rad::SparseVisMat::SparseVisMat(RadSim *pRadSim) {
	m_pRadSim = pRadSim; }

bool rad::SparseVisMat::isLoaded() { return m_bIsLoaded; }

bool rad::SparseVisMat::isValid() { return m_bIsLoaded && m_PatchHash == m_pRadSim->getPatchHash(); }

void rad::SparseVisMat::loadFromFile(const fs::path &path) {
    unloadMatrix();

    appfw::BinaryReader file(path);

    // Read magic
    char magic[sizeof(SVISMAT_MAGIC)];
    file.readBytes(magic, sizeof(SVISMAT_MAGIC));
    if (memcmp(magic, SVISMAT_MAGIC, sizeof(SVISMAT_MAGIC))) {
        logInfo("SVisMat discarded: unsupported format.");
        return;
    }

    // Read pointer size
    uint8_t ptrSize;
    file.read(ptrSize);
    if (ptrSize != sizeof(void *)) {
        logInfo("SVisMat discarded: different pointer size.");
        return;
    }

    // Read patch hash
    appfw::SHA256::Digest patchHash;
    file.readArray(appfw::span(patchHash));

    if (patchHash != m_pRadSim->m_PatchHash) {
        logInfo("SVisMat discarded: different patch hash.");
        return;
    }

    // Read patch count
    size_t patchCount = 0;
    file.read(patchCount);

    if (patchCount != m_pRadSim->m_Patches.size()) {
        logInfo("SVisMat discarded: patch count mismatch ({} instead of {}).", patchCount, m_pRadSim->m_Patches.size());
        return;
    }

    // Read list size
    size_t listSize = 0;
    file.read(listSize);

    // Allocate memory
    m_OffsetTable.resize(patchCount);
    m_CountTable.resize(patchCount);
    m_OnesCountTable.resize(patchCount);
    m_ListItems.resize(listSize);

    // Read data
    file.read(m_uTotalOnesCount);
    file.readArray(appfw::span(m_OffsetTable));
    file.readArray(appfw::span(m_CountTable));
    file.readArray(appfw::span(m_OnesCountTable));
    file.readArray(appfw::span(m_ListItems));

    m_bIsLoaded = true;
    m_PatchHash = patchHash;

    logInfo("Reusing previous vismat.");
}

void rad::SparseVisMat::saveToFile(const fs::path &path) {
    if (!m_bIsLoaded) {
        throw std::logic_error("VisMat::saveToFile: vismat not loaded");
    }

    appfw::BinaryWriter file(path);
    file.writeBytes(SVISMAT_MAGIC, sizeof(SVISMAT_MAGIC));
    file.write<uint8_t>(sizeof(void *));
    file.writeArray(appfw::span(m_PatchHash));
    file.write(m_OffsetTable.size());
    file.write(m_ListItems.size());
    file.write(m_uTotalOnesCount);
    file.writeArray(appfw::span(m_OffsetTable));
    file.writeArray(appfw::span(m_CountTable));
    file.writeArray(appfw::span(m_OnesCountTable));
    file.writeArray(appfw::span(m_ListItems));
}

void rad::SparseVisMat::buildSparseMat() {
    unloadMatrix();
    m_PatchHash = m_pRadSim->getPatchHash();

    calcSize();
    compressMat();
#if 0
    validateMat();
#endif
    
    m_bIsLoaded = true;
}

void rad::SparseVisMat::unloadMatrix() {
    m_bIsLoaded = false;
    std::vector<size_t>().swap(m_OffsetTable);
    std::vector<PatchIndex>().swap(m_CountTable);
    std::vector<PatchIndex>().swap(m_OnesCountTable);
    std::vector<ListItem>().swap(m_ListItems);
    m_uTotalOnesCount = 0;
}

void rad::SparseVisMat::calcSize() {
    logInfo("Calculating sparse matrix size...");
    appfw::Timer timer;
    timer.start();

    PatchIndex patchCount = m_pRadSim->m_Patches.size();
    auto &visdata = m_pRadSim->m_VisMat.getData();
    size_t totalListItemCount = 0;

    for (PatchIndex i = 0; i < patchCount - 1; i++) {
        PatchIndex pos = i + 1;
        size_t bitset = m_pRadSim->m_VisMat.getRowOffset(i);

        for (;;) {
            ListItem item;

            // Note:
            // (visdata[(bitset + pos) >> 3] & (1 << ((bitset + pos) & 7)))
            // <=>
            // m_pRadSim->m_VisMat.checkVisBit(i, pos)
            // Based on m_Data[bitset >> 3] |= 1 << (bitset & 7)

            // Skip zeroes
            while (!(visdata[(bitset + pos) >> 3] & (1 << ((bitset + pos) & 7))) && pos < patchCount &&
                   item.offset < item.MAX_OFFSET) {
                pos++;
                item.offset++;
            }

            // Exit if reached the end
            if (pos == patchCount) {
                break;
            }

            // Count ones
            while ((visdata[(bitset + pos) >> 3] & (1 << ((bitset + pos) & 7))) && pos < patchCount &&
                   item.size < item.MAX_SIZE) {
                pos++;
                item.size++;
            }

            // Add if not empty or if offset would overflow
            if (item.size != 0 || item.offset == item.MAX_OFFSET) {
                totalListItemCount++;
            }
        }
    }

    timer.stop();
    logInfo("Calculate sparse vismat: {:.3f} s", timer.elapsedSeconds());

    size_t vismatSize = patchCount * (sizeof(size_t) + 2 * sizeof(PatchIndex)) + totalListItemCount * sizeof(ListItem);
    logInfo("Sparse vismat size: {:.3f} MiB", vismatSize / 1024.f / 1024.f);

    m_OffsetTable.resize(patchCount);
    m_CountTable.resize(patchCount);
    m_OnesCountTable.resize(patchCount);
    m_ListItems.resize(totalListItemCount);
}

void rad::SparseVisMat::compressMat() {
    logInfo("Compress vismat into sparse matrix...");
    appfw::Timer timer;
    timer.start();

    PatchIndex patchCount = m_pRadSim->m_Patches.size();
    auto &visdata = m_pRadSim->m_VisMat.getData();
    size_t listOffset = 0;
    size_t totalOnesCount = 0; //!< How many ones there is in the table

    for (PatchIndex i = 0; i < patchCount - 1; i++) {
        PatchIndex pos = i + 1;
        size_t bitset = m_pRadSim->m_VisMat.getRowOffset(i);
        m_OffsetTable[i] = listOffset;
        PatchIndex itemCount = 0;
        PatchIndex onesCount = 0;

        for (;;) {
            ListItem item;

            // Note:
            // (visdata[(bitset + pos) >> 3] & (1 << ((bitset + pos) & 7)))
            // <=>
            // m_pRadSim->m_VisMat.checkVisBit(i, pos)
            // Based on m_Data[bitset >> 3] |= 1 << (bitset & 7)

            // Skip zeroes
            while (!(visdata[(bitset + pos) >> 3] & (1 << ((bitset + pos) & 7))) && pos < patchCount &&
                   item.offset < item.MAX_OFFSET) {
                pos++;
                item.offset++;
            }

            // Exit if reached the end
            if (pos == patchCount) {
                break;
            }

            // Count ones
            while ((visdata[(bitset + pos) >> 3] & (1 << ((bitset + pos) & 7))) && pos < patchCount &&
                   item.size < item.MAX_SIZE) {
                pos++;
                item.size++;
            }

            onesCount += item.size;

            // Add if not empty or if offset would overflow
            if (item.size != 0 || item.offset == item.MAX_OFFSET) {
                m_ListItems[listOffset] = item;
                listOffset++;
                itemCount++;
            }
        }

        m_CountTable[i] = itemCount;
        m_OnesCountTable[i] = onesCount;
        totalOnesCount += onesCount;
    }

    m_uTotalOnesCount = totalOnesCount;

    timer.stop();
    logInfo("Compress vismat: {:.1f} s", timer.elapsedSeconds());
}

void rad::SparseVisMat::validateMat() {
    logInfo("Validating sparse matrix...");
    appfw::Timer timer;
    timer.start();

    PatchIndex patchCount = m_pRadSim->m_Patches.size();

    for (PatchIndex i = 0; i < patchCount; i++) {
        // Check that vis = 0 before first item
        if (m_CountTable[i] > 0) {
            for (PatchIndex j = i + 1; j < m_ListItems[m_OffsetTable[i]].offset; j++) {
                if (m_pRadSim->m_VisMat.checkVisBit(i, j)) {
                    logFatal("sparse matrix validation failed 1: {} {}", i, j);
                    abort();
                }
            }
        }

        PatchIndex p = i + 1;

        for (size_t j = 0; j < m_CountTable[i]; j++) {
            ListItem &item = m_ListItems[m_OffsetTable[i] + j];
            p += item.offset;

            // Check that all bits are ones
            for (PatchIndex k = 0; k < item.size; k++) {
                if (!m_pRadSim->m_VisMat.checkVisBit(i, p + k)) {
                    logFatal("sparse matrix validation failed 2: {} {} {}", i, j, k);
                    abort();
                }
            }

            p += item.size;

            // Check that all bits after that are zeroes
            if (j + 1 < m_CountTable[i]) {
                for (PatchIndex k = 0; k < m_ListItems[m_OffsetTable[i] + j + 1].offset; k++) {
                    if (m_pRadSim->m_VisMat.checkVisBit(i, p + k)) {
                        logFatal("sparse matrix validation failed 3: {} {} {}", i, j, k);
                        abort();
                    }
                }
            }
        }
    }

    timer.stop();
    logInfo("Validate sparse vismat: {:.3f} s", timer.elapsedSeconds());
}
