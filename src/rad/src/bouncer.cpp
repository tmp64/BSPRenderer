#include <glm/gtx/norm.hpp>
#include "bouncer.h"
#include "rad_sim_impl.h"
#include "anorms.h"

rad::Bouncer::Bouncer(RadSimImpl &radSim)
    : m_RadSim(radSim) {
    m_flLinearThreshold = radSim.gammaToLinear(GAMMA_INTENSITY_THRESHOLD);
    m_uPatchCount = radSim.m_Patches.size();
    m_Texlights.resize(m_uPatchCount);
    m_TotalPatchLight.resize(m_uPatchCount);
}

void rad::Bouncer::setup(int lightstyle, int bounceCount) {
    m_iLightstyle = lightstyle;
    m_iBounceCount = bounceCount;
    m_PatchBounce.resize((size_t)m_uPatchCount * (size_t)(m_iBounceCount + 1));

    std::fill(m_Texlights.begin(), m_Texlights.end(), glm::vec3(0, 0, 0));
    std::fill(m_TotalPatchLight.begin(), m_TotalPatchLight.end(), glm::vec3(0, 0, 0));
    std::fill(m_PatchBounce.begin(), m_PatchBounce.end(), glm::vec3(0, 0, 0));
}

void rad::Bouncer::addSunLight() {
    glm::vec3 vSunDir = -m_RadSim.m_SunLight.vDirection;
    glm::vec3 vSunLight = m_RadSim.m_SunLight.vLight;

    if (isNullVector(vSunLight)) {
        // No sun lighting
        return;
    }

    PatchIndex patchCount = m_RadSim.m_Patches.size();

    for (PatchIndex patchIdx = 0; patchIdx < patchCount; patchIdx++) {
        PatchRef patch(m_RadSim.m_Patches, patchIdx);

        // Check if patch can be hit by the sky
        float cosangle = glm::dot(patch.getNormal(), vSunDir);

        if (cosangle < 0.0001f) {
            continue;
        }

        // Cast a ray to the sky and check if it hits
        glm::vec3 from = patch.getOrigin();
        glm::vec3 to = from + (vSunDir * SKY_RAY_LENGTH);

        if (m_RadSim.m_pLevel->traceLine(from, to) == bsp::CONTENTS_SKY) {
            // Hit the sky, add the sun color
            getPatchBounce(patchIdx, 0) += vSunLight * cosangle;
        }
    }
}

void rad::Bouncer::addSkyLight() {
    glm::vec3 vSkyLight = m_RadSim.m_SkyLight.vLight;

    if (isNullVector(vSkyLight)) {
        // No diffuse sky lighting
        return;
    }

    PatchIndex patchCount = m_RadSim.m_Patches.size();

    for (PatchIndex patchIdx = 0; patchIdx < patchCount; patchIdx++) {
        PatchRef patch(m_RadSim.m_Patches, patchIdx);

        float intensity = 0;
        const glm::vec3 normal = patch.getNormal();

        float sum = 0;

        for (size_t i = 0; i < std::size(AVER_TEX_NORMALS); i++) {
            const glm::vec3 &anorm = AVER_TEX_NORMALS[i];

            float cosangle = glm::dot(normal, anorm);

            if (cosangle < 0.0001f) {
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
            getPatchBounce(patchIdx, 0) += vSkyLight * intensity;
        }
    }
}

void rad::Bouncer::addEntLight(const EntLight &el) {
    uint8_t pvsBuf[bsp::MAX_MAP_LEAFS / 8];
    std::vector<uint8_t> litFaces(bsp::MAX_MAP_FACES);

    auto &leaves = m_RadSim.m_pLevel->getLeaves();
    auto &marksurfaces = m_RadSim.m_pLevel->getMarkSurfaces();
    unsigned leafCount = (unsigned)leaves.size();

    int lightLeaf = m_RadSim.m_pLevel->pointInLeaf(el.vOrigin);
    const uint8_t *pvs = m_RadSim.m_pLevel->leafPVS(lightLeaf, pvsBuf);

    for (unsigned leafIdx = 1; leafIdx < leafCount; leafIdx++) {
        if (!(pvs[(leafIdx - 1) >> 3] & (1 << ((leafIdx - 1) & 7)))) {
            // Leaf not visible
            continue;
        }

        const bsp::BSPLeaf &leaf = leaves[leafIdx];

        for (int i = 0; i < leaf.nMarkSurfaces; i++) {
            unsigned faceIdx = marksurfaces[leaf.iFirstMarkSurface + i];

            // Faces can be marksurfed by multiple leaves
            if (litFaces[faceIdx]) {
                continue;
            }
                
            litFaces[faceIdx] = true;
            Face &face = m_RadSim.m_Faces[faceIdx];

            if (el.type == LightType::Point) {
                addPointLightToFace(face, el);
            } else {
                std::abort();
            }
        }
    }
}

void rad::Bouncer::addTexLight(int faceIdx) {
    appfw::span<glm::vec3> patchLight = appfw::span(m_PatchBounce).subspan(0, m_uPatchCount);

    Face &face = m_RadSim.m_Faces[faceIdx];
    PatchIndex beginPatch = face.iFirstPatch;
    PatchIndex endPatch = beginPatch + face.iNumPatches;

    for (PatchIndex lightPatch = beginPatch; lightPatch < endPatch; lightPatch++) {
        m_Texlights[lightPatch] = face.vLightColor;
    }
}

void rad::Bouncer::calcLight() {
    radiateTexLights();
    bounceLight();
    calcTotalLight();
    assignLightStyles();
}

void rad::Bouncer::addPointLightToFace(Face &face, const EntLight &el) {
    PatchIndex beginPatch = face.iFirstPatch;
    PatchIndex endPatch = beginPatch + face.iNumPatches;

    glm::vec3 attenuation = glm::vec3(1, el.flLinear, el.flQuadratic) * el.flFalloff;

    for (PatchIndex patch = beginPatch; patch < endPatch; patch++) {
        PatchRef p(m_RadSim.m_Patches, patch);

        if (m_RadSim.traceLine(el.vOrigin, p.getOrigin()) != bsp::CONTENTS_EMPTY) {
            // No visibility
            continue;
        }

        // Calculate light
        glm::vec3 delta = p.getOrigin() - el.vOrigin;
        float d2 = glm::length2(delta); // dist squared
        glm::vec3 dist = glm::vec3(1, std::sqrt(d2), d2);

        float cosangle = std::max(-glm::dot(glm::normalize(delta), p.getNormal()), 0.0f);
        float k = cosangle / glm::dot(dist, attenuation);
        glm::vec3 light = k * el.vLight;
        
        // Add direct lighting
        getPatchBounce(patch, 0) += light;
    }
}

void rad::Bouncer::radiateTexLights() {
    auto &vfdata = m_RadSim.m_VFList.getVFData();
    auto &vfkoeff = m_RadSim.m_VFList.getVFKoeff();
    appfw::span<glm::vec3> initialLight = appfw::span(m_PatchBounce).subspan(0, m_uPatchCount);

    for (PatchIndex patch1 = 0; patch1 < m_uPatchCount; patch1++) {
        PatchRef patch1ref(m_RadSim.m_Patches, patch1);
        size_t dataOffset = m_RadSim.m_VFList.getPatchOffsets()[patch1];

        m_RadSim.forEachVisiblePatch(patch1, [&](PatchRef patch2ref) {
            PatchIndex patch2 = patch2ref.index();
            float vf = vfdata[dataOffset];

            initialLight[patch1] +=
                (vf * vfkoeff[patch2] * patch2ref.getSize() * patch2ref.getSize()) *
                m_Texlights[patch2];

            initialLight[patch2] +=
                (vf * vfkoeff[patch1] * patch1ref.getSize() * patch1ref.getSize()) *
                m_Texlights[patch1];

            dataOffset++;
        });
    }
}

void rad::Bouncer::bounceLight() {
    auto &vfdata = m_RadSim.m_VFList.getVFData();
    auto &vfkoeff = m_RadSim.m_VFList.getVFKoeff();

    // Calculate bounces
    for (int bounce = 1; bounce <= m_iBounceCount; bounce++) {
        appfw::span<glm::vec3> prevBounce =
            appfw::span(m_PatchBounce).subspan(m_uPatchCount * (bounce - 1), m_uPatchCount);
        appfw::span<glm::vec3> curBounce =
            appfw::span(m_PatchBounce).subspan(m_uPatchCount * bounce, m_uPatchCount);

        for (PatchIndex patch1 = 0; patch1 < m_uPatchCount; patch1++) {
            PatchRef patch1ref(m_RadSim.m_Patches, patch1);
            size_t dataOffset = m_RadSim.m_VFList.getPatchOffsets()[patch1];

            m_RadSim.forEachVisiblePatch(patch1, [&](PatchRef patch2ref) {
                PatchIndex patch2 = patch2ref.index();
                float vf = vfdata[dataOffset];

                AFW_ASSERT(!isnan(vf) && !isinf(vf));
                AFW_ASSERT(!isnan(vfkoeff[patch1]) && !isinf(vfkoeff[patch1]));
                AFW_ASSERT(!isnan(vfkoeff[patch2]) && !isinf(vfkoeff[patch2]));

                curBounce[patch1] +=
                    (vf * vfkoeff[patch2] * patch2ref.getSize() * patch2ref.getSize()) *
                    prevBounce[patch2] * patch2ref.getReflectivity();

                curBounce[patch2] +=
                    (vf * vfkoeff[patch1] * patch1ref.getSize() * patch1ref.getSize()) *
                    prevBounce[patch1] * patch1ref.getReflectivity();

                AFW_ASSERT(curBounce[patch1].r >= 0 && curBounce[patch1].g >= 0 &&
                           curBounce[patch1].b >= 0);
                AFW_ASSERT(curBounce[patch2].r >= 0 && curBounce[patch2].g >= 0 &&
                           curBounce[patch2].b >= 0);

                dataOffset++;
            });
        }
    }
}

void rad::Bouncer::calcTotalLight() {
    // Calculate radiosity result
    for (int bounce = 0; bounce <= m_iBounceCount; bounce++) {
        for (PatchIndex i = 0; i < m_uPatchCount; i++) {
            m_TotalPatchLight[i] += getPatchBounce(i, bounce);
        }
    }

    // Add texlights
    for (PatchIndex i = 0; i < m_uPatchCount; i++) {
        // TODO: May want to scale the intensity so it doesn't oversaturate
        m_TotalPatchLight[i] += m_Texlights[i];
    }
}

void rad::Bouncer::assignLightStyles() {
    // Assign lightstyles to faces
    auto &faces = m_RadSim.m_Faces;

    for (size_t faceIdx = 0; faceIdx < faces.size(); faceIdx++) {
        Face &face = faces[faceIdx];
        PatchIndex endPatch = face.iFirstPatch + face.iNumPatches;
        int lightstyleIdx = -1; // Index into face.nStyles

        for (PatchIndex i = face.iFirstPatch; i < endPatch; i++) {
            float intensity = glm::dot(m_TotalPatchLight[i], RGB_INTENSITY);

            if (intensity < m_flLinearThreshold) {
                // Patch is too dim
                continue;
            }

            // Find a valid lightstyle slot
            if (lightstyleIdx == -1) {
                for (int j = 0; j < bsp::NUM_LIGHTSTYLES; j++) {
                    if (face.nStyles[j] == 255 || face.nStyles[j] == m_iLightstyle) {
                        face.nStyles[j] = (uint8_t)m_iLightstyle;
                        lightstyleIdx = j;
                        break;
                    }
                }

                if (lightstyleIdx == -1) {
                    printe("Face #{}: too many lightstyles.", faceIdx);
                    break;
                }
            }

            // Save into the patch for sampling
            PatchRef patch(m_RadSim.m_Patches, i);
            AFW_ASSERT(isNullVector(patch.getFinalColor().color[lightstyleIdx]));
            patch.getFinalColor().color[lightstyleIdx] = m_TotalPatchLight[i];
        }
    }
}
