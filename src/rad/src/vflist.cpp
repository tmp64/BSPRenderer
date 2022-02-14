#include <appfw/timer.h>
#include <appfw/binary_file.h>
#include "rad_sim_impl.h"

rad::VFList::VFList(RadSimImpl &radSim)
    : m_RadSim(radSim) {}

bool rad::VFList::isLoaded() { return m_bIsLoaded; }

bool rad::VFList::isValid() { return m_bIsLoaded && m_PatchHash == m_RadSim.getPatchHash(); }

void rad::VFList::loadFromFile(const fs::path &path) {
    appfw::BinaryInputFile file(path);

    // Read magic
    uint8_t magic[sizeof(VF_MAGIC)];
    file.readBytes(magic, sizeof(VF_MAGIC));
    if (memcmp(magic, VF_MAGIC, sizeof(VF_MAGIC))) {
        printi("VFList discarded: unsupported format.");
        return;
    }

    // Read pointer size
    uint8_t ptrSize = file.readByte();
    if (ptrSize != sizeof(void *)) {
        printi("VFList discarded: different pointer size.");
        return;
    }

    // Read patch hash
    appfw::SHA256::Digest patchHash;
    file.readByteSpan(appfw::span(patchHash));

    if (patchHash != m_RadSim.m_PatchHash) {
        printi("VFList discarded: different patch hash.");
        return;
    }

    // Read count
    size_t patchCount = m_RadSim.m_Patches.size();
    size_t offsetSize;
    file.readObject(offsetSize);

    if (offsetSize != patchCount) {
        printi("VFList discarded: offset table size mismatch ({} instead of {}).", offsetSize, patchCount);
        return;
    }

    // Read data size
    size_t size;
    file.readObject(size);

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
    file.readObjectArray(appfw::span(m_Offsets));
    file.readObjectArray(appfw::span(m_Data));
    file.readObjectArray(appfw::span(m_Koeff));

    m_bIsLoaded = true;
    m_PatchHash = patchHash;

    printi("Reusing previous VFList.");
}

void rad::VFList::saveToFile(const fs::path &path) {
    appfw::BinaryOutputFile file(path);
    file.writeByteSpan(VF_MAGIC);
    file.writeByte(sizeof(void *));
    file.writeObjectArray(appfw::span(m_PatchHash));
    file.writeObject(m_Offsets.size());
    file.writeObject(m_Data.size());
    file.writeObjectArray(appfw::span(m_Offsets));
    file.writeObjectArray(appfw::span(m_Data));
    file.writeObjectArray(appfw::span(m_Koeff));
}

void rad::VFList::calculateVFList() {
    if (!m_RadSim.isVisMatValid()) {
        throw std::logic_error("calculateVFList: valid vismat required");
    }

    std::vector<size_t>().swap(m_Offsets);
    std::vector<float>().swap(m_Data);
    m_bIsLoaded = false;
    m_PatchHash = {};

    // Allocate memory
    PatchIndex patchCount = m_RadSim.m_Patches.size();
    size_t memoryUsage = (sizeof(size_t) + sizeof(float)) * patchCount + sizeof(float) * m_RadSim.m_SVisMat.getTotalOnesCount();
    printi("Memory required for viewfactor list: {:.3f} MiB", memoryUsage / 1024.0 / 1024.0);
    m_Offsets.resize(patchCount);
    m_Data.resize(m_RadSim.m_SVisMat.getTotalOnesCount());
    m_Koeff.resize(patchCount);

    calcOffsets();
    calcViewFactors();
    sumViewFactors();

    m_bIsLoaded = true;
    m_PatchHash = m_RadSim.getPatchHash();
}

void rad::VFList::calcOffsets() {
    printi("Calculating vflist offsets...");
    appfw::Timer timer;
    timer.start();

    PatchIndex patchCount = m_RadSim.m_Patches.size();
    size_t offset = 0;
    for (PatchIndex i = 0; i < patchCount; i++) {
        m_Offsets[i] = offset;
        offset += m_RadSim.m_SVisMat.getOnesCountTable()[i];
    }

    timer.stop();
    printi("Calculate offsets: {:.3f} s", timer.dseconds());
}

void rad::VFList::calcViewFactors() {
    printi("Calculating view factors...");

    m_RadSim.updateProgress(0);

    appfw::Timer timer;
    timer.start();

    size_t patchCount = m_RadSim.m_Patches.size();
    m_uFinishedPatches = 0;
    tf::Taskflow taskflow;
    taskflow.for_each_index_dynamic((size_t)0, patchCount, (size_t)1, [this](size_t i) { worker(i); });
    auto result = m_RadSim.m_pExecutor->run(taskflow);

    while (!appfw::isFutureReady(result)) {
        size_t work = m_uFinishedPatches;
        double done = (double)work / patchCount;
        m_RadSim.updateProgress(done);
        std::this_thread::sleep_for(std::chrono::microseconds(1000 / 30));
    }

    m_RadSim.updateProgress(1);

    timer.stop();
    printi("View factors: {:.3f} s", timer.dseconds());
}

void rad::VFList::worker(size_t i) {
    auto &countTable = m_RadSim.m_SVisMat.getCountTable();
    auto &offsetTable = m_RadSim.m_SVisMat.getOffsetTable();
    auto &listItems = m_RadSim.m_SVisMat.getListItems();

    PatchRef patch(m_RadSim.m_Patches, (PatchIndex)i);

    PatchIndex p = (PatchIndex)i + 1;
    size_t dataOffset = m_Offsets[i];

    // Calculate view factors for each visible path
    for (size_t j = 0; j < countTable[i]; j++) {
        const SparseVisMat::ListItem &item = listItems[offsetTable[i] + j];
        p += item.offset;

        for (PatchIndex k = 0; k < item.size; k++) {
            PatchRef other(m_RadSim.m_Patches, p + k);
            float vf = calcPatchViewfactor(patch, other);
            m_Data[dataOffset] = vf;
            dataOffset++;
        }

        p += item.size;
    }

    AFW_ASSERT(dataOffset - m_Offsets[i] == m_RadSim.m_SVisMat.getOnesCountTable()[i]);

    // Normalize view factors
    /*float k = 1 / sum;
    PatchIndex visPatchCount = (PatchIndex)(dataOffset - m_Offsets[i]);
    dataOffset = m_Offsets[i];
    for (size_t j = 0; j < visPatchCount; j++) {
        m_Data[dataOffset + j] *= k;
    }*/
}

void rad::VFList::sumViewFactors() {
    printi("Summing view factors...");
    appfw::Timer timer;
    timer.start();

    auto &countTable = m_RadSim.m_SVisMat.getCountTable();
    auto &offsetTable = m_RadSim.m_SVisMat.getOffsetTable();
    auto &listItems = m_RadSim.m_SVisMat.getListItems();
    PatchIndex patchCount = m_RadSim.m_Patches.size();

    for (PatchIndex i = 0; i < patchCount; i++) {
        PatchRef patchi(m_RadSim.m_Patches, i);
        PatchIndex p = i + 1;
        size_t dataOffset = m_Offsets[i];

        for (size_t j = 0; j < countTable[i]; j++) {
            const SparseVisMat::ListItem &item = listItems[offsetTable[i] + j];
            p += item.offset;

            for (PatchIndex k = 0; k < item.size; k++) {
                PatchRef patchpk(m_RadSim.m_Patches, p + k);
                float vf = m_Data[dataOffset];
                m_Koeff[i] += vf * patchpk.getSize() * patchpk.getSize();
                m_Koeff[p + k] += vf * patchi.getSize() * patchi.getSize();
                dataOffset++;
            }

            p += item.size;
        }
    }

    timer.stop();
    printi("Sum view factors: {:.3f} s", timer.dseconds());

    printi("Inversing view factor sum...");
    timer.start();

    for (PatchIndex i = 0; i < patchCount; i++) {
        if (m_Koeff[i] == 0.000000f) {
            m_Koeff[i] = 1.f;
        } else {
            m_Koeff[i] = 1.f / m_Koeff[i];
        }
    }

    timer.stop();
    printi("Inverse view factor sum: {:.3f} s", timer.dseconds());
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
