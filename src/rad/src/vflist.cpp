#include <appfw/timer.h>
#include <appfw/binary_file.h>
#include <rad/vflist.h>
#include <rad/rad_sim.h>

rad::VFList::VFList(RadSim *pRadSim) { m_pRadSim = pRadSim; }

bool rad::VFList::isLoaded() { return m_bIsLoaded; }

bool rad::VFList::isValid() { return m_bIsLoaded && m_PatchHash == m_pRadSim->getPatchHash(); }

void rad::VFList::loadFromFile(const fs::path &path) {
    appfw::BinaryReader file(path);

    // Read magic
    char magic[sizeof(VF_MAGIC)];
    file.readBytes(magic, sizeof(VF_MAGIC));
    if (memcmp(magic, VF_MAGIC, sizeof(VF_MAGIC))) {
        logInfo("VFList discarded: unsupported format.");
        return;
    }

    // Read pointer size
    uint8_t ptrSize;
    file.read(ptrSize);
    if (ptrSize != sizeof(void *)) {
        logInfo("VFList discarded: different pointer size.");
        return;
    }

    // Read patch hash
    appfw::SHA256::Digest patchHash;
    file.readArray(appfw::span(patchHash));

    if (patchHash != m_pRadSim->m_PatchHash) {
        logInfo("VFList discarded: different patch hash.");
        return;
    }

    // Read count
    size_t patchCount = m_pRadSim->m_Patches.size();
    size_t offsetSize;
    file.read(offsetSize);

    if (offsetSize != patchCount) {
        logInfo("VFList discarded: offset table size mismatch ({} instead of {}).", offsetSize, patchCount);
        return;
    }

    // Read data size
    size_t size;
    file.read(size);

    // Resize offsets
    if (m_Offsets.size() != offsetSize) {
        std::vector<size_t>().swap(m_Offsets);
        m_Offsets.resize(offsetSize);
    }

    // Resize data
    if (m_Data.size() != size) {
        std::vector<float>().swap(m_Data);
        m_Data.resize(size);
    }

    // Resize koeff
    if (m_Koeff.size() != patchCount) {
        std::vector<float>().swap(m_Koeff);
        m_Koeff.resize(patchCount);
    }

    // Read arrays
    file.readArray(appfw::span(m_Offsets));
    file.readArray(appfw::span(m_Data));
    file.readArray(appfw::span(m_Koeff));

    m_bIsLoaded = true;
    m_PatchHash = patchHash;

    logInfo("Reusing previous VFList.");
}

void rad::VFList::saveToFile(const fs::path &path) {
    appfw::BinaryWriter file(path);
    file.write(VF_MAGIC);
    file.write<uint8_t>(sizeof(void *));
    file.writeArray(appfw::span(m_PatchHash));
    file.write(m_Offsets.size());
    file.write(m_Data.size());
    file.writeArray(appfw::span(m_Offsets));
    file.writeArray(appfw::span(m_Data));
    file.writeArray(appfw::span(m_Koeff));
}

void rad::VFList::calculateVFList() {
    if (!m_pRadSim->isVisMatValid()) {
        throw std::logic_error("calculateVFList: valid vismat required");
    }

    std::vector<size_t>().swap(m_Offsets);
    std::vector<float>().swap(m_Data);
    m_bIsLoaded = false;
    m_PatchHash = {};

    // Allocate memory
    PatchIndex patchCount = m_pRadSim->m_Patches.size();
    size_t memoryUsage = (sizeof(size_t) + sizeof(float)) * patchCount + sizeof(float) * m_pRadSim->m_SVisMat.getTotalOnesCount();
    logInfo("Memory required for viewfactor list: {:.3f} MiB", memoryUsage / 1024.0 / 1024.0);
    m_Offsets.resize(patchCount);
    m_Data.resize(m_pRadSim->m_SVisMat.getTotalOnesCount());
    m_Koeff.resize(patchCount);

    calcOffsets();
    calcViewFactors();
    sumViewFactors();

    m_bIsLoaded = true;
    m_PatchHash = m_pRadSim->getPatchHash();
}

void rad::VFList::calcOffsets() {
    logInfo("Calculating vflist offsets...");
    appfw::Timer timer;
    timer.start();

    PatchIndex patchCount = m_pRadSim->m_Patches.size();
    size_t offset = 0;
    for (PatchIndex i = 0; i < patchCount; i++) {
        m_Offsets[i] = offset;
        offset += m_pRadSim->m_SVisMat.getOnesCountTable()[i];
    }

    timer.stop();
    logInfo("Calculate offsets: {:.3f} s", timer.elapsedSeconds());
}

void rad::VFList::calcViewFactors() {
    logInfo("Calculating view factors...");

    m_pRadSim->updateProgress(0);

    appfw::Timer timer;
    timer.start();

    size_t patchCount = m_pRadSim->m_Patches.size();
    m_uFinishedPatches = 0;
    tf::Taskflow taskflow;
    taskflow.for_each_index_dynamic((size_t)0, patchCount, (size_t)1, [this](size_t i) { worker(i); });
    auto result = m_pRadSim->m_Executor.run(taskflow);

    while (!appfw::IsFutureReady(result)) {
        size_t work = m_uFinishedPatches;
        double done = (double)work / patchCount;
        m_pRadSim->updateProgress(done);
        std::this_thread::sleep_for(std::chrono::microseconds(1000 / 30));
    }

    m_pRadSim->updateProgress(1);

    timer.stop();
    logInfo("View factors: {:.3f} s", timer.elapsedSeconds());
}

void rad::VFList::worker(size_t i) {
    auto &countTable = m_pRadSim->m_SVisMat.getCountTable();
    auto &offsetTable = m_pRadSim->m_SVisMat.getOffsetTable();
    auto &listItems = m_pRadSim->m_SVisMat.getListItems();

    PatchRef patch(m_pRadSim->m_Patches, (PatchIndex)i);

    PatchIndex p = (PatchIndex)i + 1;
    size_t dataOffset = m_Offsets[i];

    // Calculate view factors for each visible path
    for (size_t j = 0; j < countTable[i]; j++) {
        const SparseVisMat::ListItem &item = listItems[offsetTable[i] + j];
        p += item.offset;

        for (PatchIndex k = 0; k < item.size; k++) {
            PatchRef other(m_pRadSim->m_Patches, p + k);
            float vf = calcPatchViewfactor(patch, other);
            m_Data[dataOffset] = vf;
            dataOffset++;
        }

        p += item.size;
    }

    AFW_ASSERT(dataOffset - m_Offsets[i] == m_pRadSim->m_SVisMat.getOnesCountTable()[i]);

    // Normalize view factors
    /*float k = 1 / sum;
    PatchIndex visPatchCount = (PatchIndex)(dataOffset - m_Offsets[i]);
    dataOffset = m_Offsets[i];
    for (size_t j = 0; j < visPatchCount; j++) {
        m_Data[dataOffset + j] *= k;
    }*/
}

void rad::VFList::sumViewFactors() {
    logInfo("Summing view factors...");
    appfw::Timer timer;
    timer.start();

    auto &countTable = m_pRadSim->m_SVisMat.getCountTable();
    auto &offsetTable = m_pRadSim->m_SVisMat.getOffsetTable();
    auto &listItems = m_pRadSim->m_SVisMat.getListItems();
    PatchIndex patchCount = m_pRadSim->m_Patches.size();

    for (PatchIndex i = 0; i < patchCount; i++) {
        PatchIndex p = i + 1;
        size_t dataOffset = m_Offsets[i];

        for (size_t j = 0; j < countTable[i]; j++) {
            const SparseVisMat::ListItem &item = listItems[offsetTable[i] + j];
            p += item.offset;

            for (PatchIndex k = 0; k < item.size; k++) {
                float vf = m_Data[dataOffset];
                m_Koeff[i] += vf;
                m_Koeff[p + k] += vf;
                dataOffset++;
            }

            p += item.size;
        }
    }

    timer.stop();
    logInfo("Sum view factors: {:.3f} s", timer.elapsedSeconds());

    logInfo("Inversing view factor sum...");
    timer.start();

    for (PatchIndex i = 0; i < patchCount; i++) {
        if (m_Koeff[i] == 0.000000f) {
            m_Koeff[i] = 1.f;
        } else {
            m_Koeff[i] = 1.f / m_Koeff[i];
        }
    }

    timer.stop();
    logInfo("Inverse view factor sum: {:.3f} s", timer.elapsedSeconds());
}

float rad::VFList::calcPatchViewfactor(PatchRef &patch1, PatchRef &patch2) {
    glm::vec3 dir = patch2.getOrigin() - patch1.getOrigin();
    float dist = glm::length(dir);

    if (floatEquals(dist, 0)) {
        return 0;
    }

    dir = glm::normalize(dir);
    float cos1 = glm::dot(patch1.getNormal(), dir);
    float cos2 = -glm::dot(patch2.getNormal(), dir);

    if (cos1 < 0 || cos2 < 0) {
        return 0;
    }

    float viewFactor = cos1 * cos2 * 10 / (dist * dist);
    AFW_ASSERT(!isnan(viewFactor));
    AFW_ASSERT(viewFactor >= 0);

    /*if (viewFactor * 10000 < 0.05f) {
        // Skip this patch
        return 0;
    }*/

    return viewFactor;
}
