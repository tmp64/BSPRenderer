#include <rad/bouncer.h>
#include <rad/rad_sim.h>

void rad::Bouncer::setup(RadSim *pRadSim, int bounceCount) {
    m_pRadSim = pRadSim;
    m_iBounceCount = bounceCount;
    m_uPatchCount = m_pRadSim->m_Patches.size();

    m_PatchBounce.resize((size_t)m_uPatchCount * (size_t)(m_iBounceCount + 1));
    m_PatchSum.resize(m_uPatchCount);
    std::fill(m_PatchBounce.begin(), m_PatchBounce.end(), glm::vec3(0, 0, 0));
}

void rad::Bouncer::addPatchLight(PatchIndex patch, const glm::vec3 &light) {
    getPatchBounce(patch, 0) += light;
}

void rad::Bouncer::bounceLight() {
    float refl = m_pRadSim->m_flReflectivity;

    // Copy bounce 0 into the lightmap
    for (PatchIndex i = 0; i < m_uPatchCount; i++) {
        PatchRef patch(m_pRadSim->m_Patches, i);
        patch.getFinalColor() = getPatchBounce(i, 0);
    }

    m_pRadSim->updateProgress(0);

    // Bounce light
    for (int bounce = 1; bounce <= m_iBounceCount; bounce++) {
        m_pRadSim->updateProgress((bounce - 1.0) / (m_iBounceCount - 1.0));

        std::fill(m_PatchSum.begin(), m_PatchSum.end(), glm::vec3(0, 0, 0));

        // Bounce light for each visible path
        tf::Taskflow taskflow;
        tf::Task taskThis = taskflow.for_each_index_dynamic(
            (PatchIndex)0, m_uPatchCount, (PatchIndex)1,
            [&](PatchIndex idx) { receiveLightFromThis(bounce, idx); }, (PatchIndex)128);

        tf::Task taskOther = taskflow.for_each_index_dynamic(
            (PatchIndex)0, m_uPatchCount, (PatchIndex)1,
            [&](PatchIndex idx) { receiveLightFromOther(bounce, idx); }, (PatchIndex)128);

        taskThis.precede(taskOther);
        m_pRadSim->m_Executor.run(taskflow).wait();

        for (PatchIndex i = 0; i < m_uPatchCount; i++) {
            // Received light from all visible patches
            // Write it
            PatchRef patch(m_pRadSim->m_Patches, i);
            AFW_ASSERT(m_PatchSum[i].r >= 0 && m_PatchSum[i].g >= 0 && m_PatchSum[i].b >= 0);
            getPatchBounce(i, bounce) = m_PatchSum[i] * refl;
            patch.getFinalColor() += getPatchBounce(i, bounce);
        }
    }

    m_pRadSim->updateProgress(1);
}

void rad::Bouncer::receiveLightFromThis(int bounce, PatchIndex i) {
    auto &countTable = m_pRadSim->m_SVisMat.getCountTable();
    auto &offsetTable = m_pRadSim->m_SVisMat.getOffsetTable();
    auto &listItems = m_pRadSim->m_SVisMat.getListItems();
    auto &vfoffsets = m_pRadSim->m_VFList.getPatchOffsets();
    auto &vfdata = m_pRadSim->m_VFList.getVFData();
    auto &vfkoeff = m_pRadSim->m_VFList.getVFKoeff();

    PatchRef patch(m_pRadSim->m_Patches, i);
    PatchIndex poff = i + 1;
    size_t dataOffset = vfoffsets[i];

    for (size_t j = 0; j < countTable[i]; j++) {
        const SparseVisMat::ListItem &item = listItems[offsetTable[i] + j];
        poff += item.offset;

        for (PatchIndex k = 0; k < item.size; k++) {
            PatchIndex patch2 = poff + k;
            float vf = vfdata[dataOffset];

            AFW_ASSERT(!isnan(vf) && !isinf(vf));
            AFW_ASSERT(!isnan(vfkoeff[i]) && !isinf(vfkoeff[i]));
            AFW_ASSERT(!isnan(vfkoeff[patch2]) && !isinf(vfkoeff[patch2]));

            // Take light from i to patch2
            m_PatchSum[patch2] += getPatchBounce(i, bounce - 1) * (vf * vfkoeff[patch2]);

            AFW_ASSERT(m_PatchSum[i].r >= 0 && m_PatchSum[i].g >= 0 && m_PatchSum[i].b >= 0);
            AFW_ASSERT(m_PatchSum[patch2].r >= 0 && m_PatchSum[patch2].g >= 0 &&
                       m_PatchSum[patch2].b >= 0);

            dataOffset++;
        }

        poff += item.size;
    }
}

void rad::Bouncer::receiveLightFromOther(int bounce, PatchIndex i) {
    auto &countTable = m_pRadSim->m_SVisMat.getCountTable();
    auto &offsetTable = m_pRadSim->m_SVisMat.getOffsetTable();
    auto &listItems = m_pRadSim->m_SVisMat.getListItems();
    auto &vfoffsets = m_pRadSim->m_VFList.getPatchOffsets();
    auto &vfdata = m_pRadSim->m_VFList.getVFData();
    auto &vfkoeff = m_pRadSim->m_VFList.getVFKoeff();

    PatchRef patch(m_pRadSim->m_Patches, i);
    PatchIndex poff = i + 1;
    size_t dataOffset = vfoffsets[i];

    for (size_t j = 0; j < countTable[i]; j++) {
        const SparseVisMat::ListItem &item = listItems[offsetTable[i] + j];
        poff += item.offset;

        for (PatchIndex k = 0; k < item.size; k++) {
            PatchIndex patch2 = poff + k;
            float vf = vfdata[dataOffset];

            AFW_ASSERT(!isnan(vf) && !isinf(vf));
            AFW_ASSERT(!isnan(vfkoeff[i]) && !isinf(vfkoeff[i]));
            AFW_ASSERT(!isnan(vfkoeff[patch2]) && !isinf(vfkoeff[patch2]));

            // Take light from patch2 to i
            m_PatchSum[i] += getPatchBounce(patch2, bounce - 1) * (vf * vfkoeff[i]);

            AFW_ASSERT(m_PatchSum[i].r >= 0 && m_PatchSum[i].g >= 0 && m_PatchSum[i].b >= 0);
            AFW_ASSERT(m_PatchSum[patch2].r >= 0 && m_PatchSum[patch2].g >= 0 &&
                       m_PatchSum[patch2].b >= 0);

            dataOffset++;
        }

        poff += item.size;
    }
}
