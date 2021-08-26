#include <vector>
#include <forward_list>
#include <appfw/timer.h>
#include <appfw/platform.h>
#include <appfw/binary_file.h>
#include "rad_sim_impl.h"

rad::SparseVisMat::SparseVisMat(RadSimImpl &radSim)
    : m_RadSim(radSim) {}

bool rad::SparseVisMat::isLoaded() { return m_bIsLoaded; }

bool rad::SparseVisMat::isValid() { return m_bIsLoaded && m_PatchHash == m_RadSim.getPatchHash(); }

void rad::SparseVisMat::loadFromFile(const fs::path &path) {
    unloadMatrix();

    appfw::BinaryInputFile file(path);

    // Read magic
    uint8_t magic[sizeof(SVISMAT_MAGIC)];
    file.readBytes(magic, sizeof(SVISMAT_MAGIC));
    if (memcmp(magic, SVISMAT_MAGIC, sizeof(SVISMAT_MAGIC))) {
        printi("SVisMat discarded: unsupported format.");
        return;
    }

    // Read pointer size
    uint8_t ptrSize;
    ptrSize = file.readByte();
    if (ptrSize != sizeof(void *)) {
        printi("SVisMat discarded: different pointer size.");
        return;
    }

    // Read patch hash
    appfw::SHA256::Digest patchHash;
    file.readByteSpan(appfw::span(patchHash));

    if (patchHash != m_RadSim.m_PatchHash) {
        printi("SVisMat discarded: different patch hash.");
        return;
    }

    // Read patch count
    size_t patchCount = 0;
    file.readObject(patchCount);

    if (patchCount != m_RadSim.m_Patches.size()) {
        printi("SVisMat discarded: patch count mismatch ({} instead of {}).", patchCount, m_RadSim.m_Patches.size());
        return;
    }

    // Read list size
    size_t listSize = 0;
    file.readObject(listSize);

    // Allocate memory
    m_OffsetTable.resize(patchCount);
    m_CountTable.resize(patchCount);
    m_OnesCountTable.resize(patchCount);
    m_ListItems.resize(listSize);

    // Read data
    file.readObject(m_uTotalOnesCount);
    file.readObjectArray(appfw::span(m_OffsetTable));
    file.readObjectArray(appfw::span(m_CountTable));
    file.readObjectArray(appfw::span(m_OnesCountTable));
    file.readObjectArray(appfw::span(m_ListItems));

    m_bIsLoaded = true;
    m_PatchHash = patchHash;

    printi("Reusing previous vismat.");
}

void rad::SparseVisMat::saveToFile(const fs::path &path) {
    if (!m_bIsLoaded) {
        throw std::logic_error("VisMat::saveToFile: vismat not loaded");
    }

    appfw::BinaryOutputFile file(path);
    file.writeBytes(SVISMAT_MAGIC, sizeof(SVISMAT_MAGIC));
    file.writeByte(sizeof(void *));
    file.writeByteSpan(appfw::span(m_PatchHash));
    file.writeObject(m_OffsetTable.size());
    file.writeObject(m_ListItems.size());
    file.writeObject(m_uTotalOnesCount);
    file.writeObjectArray(appfw::span(m_OffsetTable));
    file.writeObjectArray(appfw::span(m_CountTable));
    file.writeObjectArray(appfw::span(m_OnesCountTable));
    file.writeObjectArray(appfw::span(m_ListItems));
}

void rad::SparseVisMat::buildSparseMat() {
    unloadMatrix();
    m_PatchHash = m_RadSim.getPatchHash();

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
    printn("Calculating sparse matrix size...");
    appfw::Timer timer;
    timer.start();

    PatchIndex patchCount = m_RadSim.m_Patches.size();
    auto &visdata = m_RadSim.m_VisMat.getData();
    std::atomic_size_t totalListItemCount = 0;

    auto fnLoopBody = [&](PatchIndex i) {
        PatchIndex pos = i + 1;
        size_t bitset = m_RadSim.m_VisMat.getRowOffset(i);

        for (;;) {
            ListItem item;

            // Note:
            // (visdata[(bitset + pos) >> 3] & (1 << ((bitset + pos) & 7)))
            // <=>
            // m_RadSim.m_VisMat.checkVisBit(i, pos)
            // Based on m_Data[bitset >> 3] |= 1 << (bitset & 7)

            // Skip zeroes
            while (!(visdata[(bitset + pos) >> 3] & (1 << ((bitset + pos) & 7))) &&
                   pos < patchCount && item.offset < item.MAX_OFFSET) {
                pos++;
                item.offset++;
            }

            // Exit if reached the end
            if (pos == patchCount) {
                break;
            }

            // Count ones
            while ((visdata[(bitset + pos) >> 3] & (1 << ((bitset + pos) & 7))) &&
                   pos < patchCount && item.size < item.MAX_SIZE) {
                pos++;
                item.size++;
            }

            // Add if not empty or if offset would overflow
            if (item.size != 0 || item.offset == item.MAX_OFFSET) {
                totalListItemCount++;
            }
        }
    };

    tf::Taskflow taskflow;
    taskflow.for_each_index_dynamic((PatchIndex)0, patchCount - 1, (PatchIndex)1, fnLoopBody);
    m_RadSim.m_Executor.run(taskflow).wait();

    timer.stop();
    printi("Calculate sparse vismat: {:.3f} s", timer.dseconds());

    size_t vismatSize = patchCount * (sizeof(size_t) + 2 * sizeof(PatchIndex)) + totalListItemCount * sizeof(ListItem);
    printi("Sparse vismat size: {:.3f} MiB", vismatSize / 1024.f / 1024.f);

    m_OffsetTable.resize(patchCount);
    m_CountTable.resize(patchCount);
    m_OnesCountTable.resize(patchCount);
    m_ListItems.resize(totalListItemCount);
}

void rad::SparseVisMat::compressMat() {
    printi("Compress vismat into sparse matrix...");
    appfw::Timer timer;
    timer.start();

    PatchIndex patchCount = m_RadSim.m_Patches.size();
    auto &visdata = m_RadSim.m_VisMat.getData();
    size_t listOffset = 0;
    size_t totalOnesCount = 0; //!< How many ones there is in the table

    for (PatchIndex i = 0; i < patchCount - 1; i++) {
        PatchIndex pos = i + 1;
        size_t bitset = m_RadSim.m_VisMat.getRowOffset(i);
        m_OffsetTable[i] = listOffset;
        PatchIndex itemCount = 0;
        PatchIndex onesCount = 0;

        for (;;) {
            ListItem item;

            // Note:
            // (visdata[(bitset + pos) >> 3] & (1 << ((bitset + pos) & 7)))
            // <=>
            // m_RadSim.m_VisMat.checkVisBit(i, pos)
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
    printi("Compress vismat: {:.1f} s", timer.dseconds());
}

void rad::SparseVisMat::validateMat() {
    printi("Validating sparse matrix...");
    appfw::Timer timer;
    timer.start();

    PatchIndex patchCount = m_RadSim.m_Patches.size();

    for (PatchIndex i = 0; i < patchCount; i++) {
        // Check that vis = 0 before first item
        if (m_CountTable[i] > 0) {
            for (PatchIndex j = i + 1; j < m_ListItems[m_OffsetTable[i]].offset; j++) {
                if (m_RadSim.m_VisMat.checkVisBit(i, j)) {
                    printfatal("sparse matrix validation failed 1: {} {}", i, j);
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
                if (!m_RadSim.m_VisMat.checkVisBit(i, p + k)) {
                    printfatal("sparse matrix validation failed 2: {} {} {}", i, j, k);
                    abort();
                }
            }

            p += item.size;

            // Check that all bits after that are zeroes
            if (j + 1 < m_CountTable[i]) {
                for (PatchIndex k = 0; k < m_ListItems[m_OffsetTable[i] + j + 1].offset; k++) {
                    if (m_RadSim.m_VisMat.checkVisBit(i, p + k)) {
                        printfatal("sparse matrix validation failed 3: {} {} {}", i, j, k);
                        abort();
                    }
                }
            }
        }
    }

    timer.stop();
    printi("Validate sparse vismat: {:.3f} s", timer.dseconds());
}
