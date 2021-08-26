#include "bouncer.h"
#include "rad_sim_impl.h"
#include "anorms.h"

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

void rad::Bouncer::setup(int bounceCount) {
    m_iBounceCount = bounceCount;
    m_uPatchCount = m_RadSim.m_Patches.size();

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
    float refl = m_RadSim.m_Config.flBaseRefl * m_RadSim.m_LevelConfig.flRefl;

    // Copy bounce 0 into the lightmap
    for (PatchIndex i = 0; i < m_uPatchCount; i++) {
        PatchRef patch(m_RadSim.m_Patches, i);
        patch.getFinalColor() = getPatchBounce(i, 0);
    }

    m_RadSim.updateProgress(0);

    // Bounce light
    for (int bounce = 1; bounce <= m_iBounceCount; bounce++) {
        m_RadSim.updateProgress((bounce - 1.0) / (m_iBounceCount - 1.0));

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
        m_RadSim.m_Executor.run(taskflow).wait();

        for (PatchIndex i = 0; i < m_uPatchCount; i++) {
            // Received light from all visible patches
            // Write it
            PatchRef patch(m_RadSim.m_Patches, i);
            AFW_ASSERT(m_PatchSum[i].r >= 0 && m_PatchSum[i].g >= 0 && m_PatchSum[i].b >= 0);
            getPatchBounce(i, bounce) = m_PatchSum[i] * refl;
            patch.getFinalColor() += getPatchBounce(i, bounce);
        }
    }

    m_RadSim.updateProgress(1);
}

void rad::Bouncer::addEnvLighting() {
    const SunLight &sun = m_RadSim.m_LevelConfig.sunLight;
    const SkyLight &sky = m_RadSim.m_LevelConfig.skyLight;
    
    bool bIsSunSet = sun.bIsSet;

    glm::vec3 vSunDir = -getVecFromAngles(sun.flPitch, sun.flYaw);
    glm::vec3 vSunColor = m_RadSim.gammaToLinear(sun.vColor) * sun.flBrightness;
    glm::vec3 vSkyColor;

    if (sky.vColor.r != -1) {
        vSkyColor = m_RadSim.gammaToLinear(sky.vColor) * sun.flBrightness;
    } else {
        vSkyColor = vSunColor;
    }

    vSkyColor *= sky.flBrightnessMul;

    if (!bIsSunSet && vSkyColor == glm::vec3(0, 0, 0)) {
        // No env lighting
        return;
    }

    PatchIndex patchCount = m_RadSim.m_Patches.size();

    for (PatchIndex patchIdx = 0; patchIdx < patchCount; patchIdx++) {
        PatchRef patch(m_RadSim.m_Patches, patchIdx);

        if (bIsSunSet) {
            addDirectSunlight(patch, vSunDir, vSunColor);
        }

        addDiffuseSkylight(patch, vSkyColor);
    }
}

void rad::Bouncer::addTexLights() {
    for (Face &face : m_RadSim.m_Faces) {
        const bsp::BSPTextureInfo &texinfo = m_RadSim.m_pLevel->getTexInfo()[face.iTextureInfo];
        const bsp::BSPMipTex &miptex = m_RadSim.m_pLevel->getTextures()[texinfo.iMiptex];

        // TODO: Actual texinfo
        if (miptex.szName[0] == '~') {
            for (PatchIndex p = face.iFirstPatch; p < face.iFirstPatch + face.iNumPatches; p++) {
                addPatchLight(p, glm::vec3(1, 1, 1) * 50.f);
            }
        }
    }
}

void rad::Bouncer::addDirectSunlight(PatchRef &patch, const glm::vec3 vSunDir,
                                     const glm::vec3 &vSunColor) {
    // Check if patch can be hit by the sky
    float cosangle = glm::dot(patch.getNormal(), vSunDir);

    if (cosangle < 0.001f) {
        return;
    }

    // Cast a ray to the sky and check if it hits
    glm::vec3 from = patch.getOrigin();
    glm::vec3 to = from + (vSunDir * SKY_RAY_LENGTH);

    if (m_RadSim.m_pLevel->traceLine(from, to) == bsp::CONTENTS_SKY) {
        // Hit the sky, add the sun color
        addPatchLight(patch.index(), vSunColor * cosangle);
    }
}

void rad::Bouncer::addDiffuseSkylight(PatchRef &patch, const glm::vec3 &vSkyColor) {
    float intensity = 0;
    const glm::vec3 normal = patch.getNormal();

    float sum = 0;

    for (size_t i = 0; i < std::size(AVER_TEX_NORMALS); i++) {
        const glm::vec3 &anorm = AVER_TEX_NORMALS[i];

        float cosangle = -glm::dot(normal, anorm);

        if (cosangle < 0.001f) {
            continue;
        }

        sum += cosangle;

        // Cast a ray to the sky and check if it hits
        glm::vec3 from = patch.getOrigin();
        glm::vec3 to = from + (anorm * SKY_RAY_LENGTH);

        if (m_RadSim.m_pLevel->traceLine(from, to) == bsp::CONTENTS_SKY) {
            // Hit the sky
            intensity += cosangle;
        }
    }

    if (sum != 0) {
        intensity /= sum;
        addPatchLight(patch.index(), vSkyColor * intensity);
    }
}

void rad::Bouncer::receiveLightFromThis(int bounce, PatchIndex i) {
    auto &countTable = m_RadSim.m_SVisMat.getCountTable();
    auto &offsetTable = m_RadSim.m_SVisMat.getOffsetTable();
    auto &listItems = m_RadSim.m_SVisMat.getListItems();
    auto &vfoffsets = m_RadSim.m_VFList.getPatchOffsets();
    auto &vfdata = m_RadSim.m_VFList.getVFData();
    auto &vfkoeff = m_RadSim.m_VFList.getVFKoeff();

    PatchRef patch(m_RadSim.m_Patches, i);
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
    auto &countTable = m_RadSim.m_SVisMat.getCountTable();
    auto &offsetTable = m_RadSim.m_SVisMat.getOffsetTable();
    auto &listItems = m_RadSim.m_SVisMat.getListItems();
    auto &vfoffsets = m_RadSim.m_VFList.getPatchOffsets();
    auto &vfdata = m_RadSim.m_VFList.getVFData();
    auto &vfkoeff = m_RadSim.m_VFList.getVFKoeff();

    PatchRef patch(m_RadSim.m_Patches, i);
    PatchIndex poff = i + 1;
    size_t dataOffset = vfoffsets[i];

    for (size_t j = 0; j < countTable[i]; j++) {
        const SparseVisMat::ListItem &item = listItems[offsetTable[i] + j];
        poff += item.offset;

        for (PatchIndex k = 0; k < item.size; k++) {
            PatchIndex patch2 = poff + k;
            PatchRef patch2ref(m_RadSim.m_Patches, patch2);
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
