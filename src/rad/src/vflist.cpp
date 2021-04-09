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
    size_t count;
    size_t patchCount = m_pRadSim->m_Patches.size();
    file.read(count);

    if (count != patchCount) {
        logInfo("VFList discarded: invalid patch count ({} instead of {}).", count, patchCount);
        return;
    }

    // Read data size
    size_t size;
    file.read(size);

    // Resize count
    if (m_VFCount.size() != count) {
        std::vector<PatchIndex>().swap(m_VFCount);
        m_VFCount.resize(count);
    }

    // Resize data
    if (m_Data.size() != size) {
        std::vector<ViewFactor>().swap(m_Data);
        m_Data.resize(size);
    }

    // Read data
    file.readArray(appfw::span(m_VFCount));

    for (size_t i = 0; i < m_Data.size(); i++) {
        file.read(std::get<0>(m_Data[i]));
        file.read(std::get<1>(m_Data[i]));
    }

    // Read viewfactor offsets
    for (PatchIndex i = 0; i < m_pRadSim->m_Patches.size(); i++) {
        PatchRef patch(m_pRadSim->m_Patches, i);
        size_t offset;
        size_t len;
        file.read(offset);
        file.read(len);
        patch.getViewFactors() = appfw::span(m_Data).subspan(offset, len);
    }

    m_bIsLoaded = true;
    m_PatchHash = patchHash;

    logInfo("Reusing previous VFList.");
}

void rad::VFList::saveToFile(const fs::path &path) {
    appfw::BinaryWriter file(path);
    file.write(VF_MAGIC);
    file.write<uint8_t>(sizeof(void *));
    file.writeArray(appfw::span(m_PatchHash));
    file.write(m_VFCount.size());
    file.write(m_Data.size());
    file.writeArray(appfw::span(m_VFCount));

    // Write viewvactors
    for (size_t i = 0; i < m_Data.size(); i++) {
        file.write(std::get<0>(m_Data[i]));
        file.write(std::get<1>(m_Data[i]));
    }

    // Write viewfactor offsets
    for (PatchIndex i = 0; i < m_pRadSim->m_Patches.size(); i++) {
        PatchRef patch(m_pRadSim->m_Patches, i);
        auto &span = patch.getViewFactors();
        size_t offset = span.data() - m_Data.data();
        size_t size = span.size();
        file.write(offset);
        file.write(size);
    }
}

void rad::VFList::calculateVFList() {
    if (!m_pRadSim->isVisMatValid()) {
        throw std::logic_error("calculateVFList: valid vismat required");
    }

    std::vector<PatchIndex>().swap(m_VFCount);
    std::vector<ViewFactor>().swap(m_Data);
    m_bIsLoaded = false;
    m_PatchHash = {};

    calcCount();
    calcViewFactors();

    m_bIsLoaded = true;
    m_PatchHash = m_pRadSim->getPatchHash();
}

void rad::VFList::calcCount() {
	logInfo("Counting visible patches...");

	appfw::Timer timer;
    timer.start();

    size_t totalCount = 0;
    m_VFCount.resize(m_pRadSim->m_Patches.size());

    m_pRadSim->updateProgress(0);

    for (PatchIndex i = 0; i < m_pRadSim->m_Patches.size(); i++) {
        m_pRadSim->updateProgress((double)i / m_pRadSim->m_Patches.size());

        for (PatchIndex j = i + 1; j < m_pRadSim->m_Patches.size(); j++) {
            if (m_pRadSim->m_VisMat.checkVisBit(i, j)) {
                m_VFCount[i] += 1;
                m_VFCount[j] += 1;
                totalCount += 2;
            }
        }
    }

    m_pRadSim->updateProgress(1);

    timer.stop();
    logInfo("Counted patches in {:.3} s", timer.elapsedSeconds());
    logInfo("Memory required for viewfactor list: {:.3f} MiB", sizeof(ViewFactor) * totalCount / 1024.0 / 1024.0);

    logInfo("Allocating memory...");
    m_Data.resize(totalCount);

    logInfo("Writing pointers into patches...");
    setPointers();
}

void rad::VFList::setPointers() {
    appfw::span<ViewFactor> span(m_Data);
    size_t nextIdx = 0;
    for (PatchIndex i = 0; i < m_pRadSim->m_Patches.size(); i++) {
        PatchRef patch(m_pRadSim->m_Patches, i);
        patch.getViewFactors() = span.subspan(nextIdx, m_VFCount[i]);
        nextIdx += m_VFCount[i];
    }
}

void rad::VFList::calcViewFactors() {
    logInfo("Calculating view factors...");

    m_pRadSim->updateProgress(0);

    appfw::Timer timer;
    timer.start();

    m_pRadSim->m_Dispatcher.setWorkCount(m_pRadSim->m_Patches.size());
    m_pRadSim->m_ThreadPool.run([this](auto &ti) { worker(ti); }, 0);

    while (!m_pRadSim->m_ThreadPool.isFinished()) {
        double done = (double)m_pRadSim->m_ThreadPool.getStatus() / m_pRadSim->m_Patches.size();
        m_pRadSim->updateProgress(done);
        std::this_thread::sleep_for(m_pRadSim->m_ThreadPool.POLL_DELAY);
    }

    if (m_pRadSim->m_ThreadPool.getStatus() != m_pRadSim->m_Patches.size()) {
        logError("Threading failure!");
        abort();
    }

    m_pRadSim->updateProgress(1);

    timer.stop();
    logInfo("View factors: {:.3} s", timer.elapsedSeconds());
}

void rad::VFList::worker(appfw::ThreadPool::ThreadInfo &ti) {
    PatchIndex patchCount = m_pRadSim->m_Patches.size();

    for (size_t i = m_pRadSim->m_Dispatcher.getWork(); i != m_pRadSim->m_Dispatcher.WORK_DONE;
         i = m_pRadSim->m_Dispatcher.getWork(), ti.iWorkDone++) {
        PatchRef patch(m_pRadSim->m_Patches, (PatchIndex)i);
        float sum = 0;
        PatchIndex viewFactorIdx = 0;

        for (PatchIndex j = 0; j < patchCount; j++) {
            if (i == j) {
                continue;
            }

            PatchRef other(m_pRadSim->m_Patches, j);

            glm::vec3 dir = other.getOrigin() - patch.getOrigin();
            float dist = glm::length(dir);

            if (floatEquals(dist, 0)) {
                continue;
            }

            if (!m_pRadSim->m_VisMat.checkVisBit((PatchIndex)i, j)) {
                // Patches can't see each other
                continue;
            }

            dir = glm::normalize(dir);
            float cos1 = glm::dot(patch.getNormal(), dir);
            float cos2 = -glm::dot(other.getNormal(), dir);

            if (cos1 < 0 || cos2 < 0) {
                continue;
            }

            float viewFactor = cos1 * cos2 * 10 / (dist * dist);
            AFW_ASSERT(!isnan(viewFactor));
            AFW_ASSERT(viewFactor >= 0);

            if (viewFactor * 10000 < 0.05f) {
                // Skip this patch
                continue;
            }

            patch.getViewFactors()[viewFactorIdx] = {j, viewFactor};
            sum += viewFactor;
            viewFactorIdx++;
        }

        m_VFCount[i] = viewFactorIdx;

        // Normalize view factors
        float k = 1 / sum;
        for (size_t j = 0; j < viewFactorIdx; j++) {
            std::get<1>(patch.getViewFactors()[j]) *= k;
        }
    }
}
