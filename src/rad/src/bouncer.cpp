#include <rad/bouncer.h>
#include <rad/rad_sim.h>

/**
 * Returns vector that points in the direction given in degrees (or -1/-2 for yaw).
 * Used for sunlight calculation.
 */
static glm::vec3 getVecFromAngles(float pitch, float yaw) {
    glm::vec3 dir;

    if (yaw == -1) {
        // ANGLE_UP
        dir.x = 0;
        dir.y = 0;
        dir.z = 1;
    } else if (yaw == -2) {
        // ANGLE_DOWN
        dir.x = 0;
        dir.y = 0;
        dir.z = -1;
    } else {
        dir.x = cos(glm::radians(yaw));
        dir.y = sin(glm::radians(yaw));
        dir.z = 0;
    }

    dir.x *= cos(glm::radians(pitch));
    dir.y *= cos(glm::radians(pitch));
    dir.z = sin(glm::radians(pitch));

    return glm::normalize(dir);
}

void rad::Bouncer::setup(RadSim *pRadSim, int bounceCount) {
    m_pRadSim = pRadSim;
    m_iBounceCount = bounceCount;
    m_uPatchCount = m_pRadSim->m_Patches.size();

    m_PatchBounce.resize((size_t)m_uPatchCount * (size_t)(m_iBounceCount + 1));
    m_PatchSum.resize(m_uPatchCount);
    std::fill(m_PatchBounce.begin(), m_PatchBounce.end(), glm::vec3(0, 0, 0));

    addLighting();
}

void rad::Bouncer::addLighting() {
    addEnvLighting();
    addTexLights();
}

void rad::Bouncer::addPatchLight(PatchIndex patch, const glm::vec3 &light) {
    getPatchBounce(patch, 0) += light;
}

void rad::Bouncer::bounceLight() {
    float refl = m_pRadSim->m_Config.flBaseRefl * m_pRadSim->m_LevelConfig.flRefl;

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

void rad::Bouncer::addEnvLighting() {
    const SunLight &sun = m_pRadSim->m_LevelConfig.sunLight;
    
    if (!sun.bIsSet) {
        return;
    }

    glm::vec3 vSunDir = -getVecFromAngles(sun.flPitch, sun.flYaw);
    glm::vec3 vSunColor = m_pRadSim->gammaToLinear(sun.vColor) * sun.flBrightness;

    PatchIndex patchCount = m_pRadSim->m_Patches.size();

    for (PatchIndex patchIdx = 0; patchIdx < patchCount; patchIdx++) {
        PatchRef patch(m_pRadSim->m_Patches, patchIdx);

        // Check if patch can be hit by the sky
        float cosangle = glm::dot(patch.getNormal(), vSunDir);

        if (cosangle < 0.001f) {
            continue;
        }

        // Cast a ray to the sky and check if it hits
        glm::vec3 from = patch.getOrigin();
        glm::vec3 to = from + (vSunDir * SKY_RAY_LENGTH);

        if (m_pRadSim->m_pLevel->traceLine(from, to) == bsp::CONTENTS_SKY) {
            // Hit the sky, add the sun color
            addPatchLight(patchIdx, vSunColor * cosangle);
        }
    }
}

void rad::Bouncer::addTexLights() {
    for (Face &face : m_pRadSim->m_Faces) {
        const bsp::BSPTextureInfo &texinfo = m_pRadSim->m_pLevel->getTexInfo()[face.iTextureInfo];
        const bsp::BSPMipTex &miptex = m_pRadSim->m_pLevel->getTextures()[texinfo.iMiptex];

        // TODO: Actual texinfo
        if (miptex.szName[0] == '~') {
            for (PatchIndex p = face.iFirstPatch; p < face.iFirstPatch + face.iNumPatches; p++) {
                addPatchLight(p, glm::vec3(1, 1, 1) * 50.f);
            }
        }
    }
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
            m_PatchSum[patch2] += getPatchBounce(i, bounce - 1) *
                                  (vf * vfkoeff[patch2] * patch.getSize() * patch.getSize());

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
            PatchRef patch2ref(m_pRadSim->m_Patches, patch2);
            float vf = vfdata[dataOffset];

            AFW_ASSERT(!isnan(vf) && !isinf(vf));
            AFW_ASSERT(!isnan(vfkoeff[i]) && !isinf(vfkoeff[i]));
            AFW_ASSERT(!isnan(vfkoeff[patch2]) && !isinf(vfkoeff[patch2]));

            // Take light from patch2 to i
            m_PatchSum[i] += getPatchBounce(patch2, bounce - 1) *
                             (vf * vfkoeff[i] * patch2ref.getSize() * patch2ref.getSize());

            AFW_ASSERT(m_PatchSum[i].r >= 0 && m_PatchSum[i].g >= 0 && m_PatchSum[i].b >= 0);
            AFW_ASSERT(m_PatchSum[patch2].r >= 0 && m_PatchSum[patch2].g >= 0 &&
                       m_PatchSum[patch2].b >= 0);

            dataOffset++;
        }

        poff += item.size;
    }
}
