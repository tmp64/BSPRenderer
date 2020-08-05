#include <appfw/binary_file.h>
#include <appfw/timer.h>
#include <appfw/services.h>
#include "patches.h"
#include "main.h"
#include "utils.h"
#include "bsp_tree.h"

static std::vector<glm::vec3> s_PatchBounce;

static inline glm::vec3 &getPatchBounce(size_t patch, size_t bounce) {
    AFW_ASSERT(patch < g_Patches.size() && bounce <= g_Config.iBounceCount);
    return s_PatchBounce[patch * (g_Config.iBounceCount + 1) + bounce];
}

#if 0
// Unused
static glm::vec3 randomColor() {
    glm::vec3 out;
    out.r = (rand() % 256) / 255.f;
    out.g = (rand() % 256) / 255.f;
    out.b = (rand() % 256) / 255.f;
    return out;
}
#endif

void createPatches() {
    logInfo("Creating patches...");

    appfw::Timer timer;
    timer.start();

    size_t pixelCount = 0;
    size_t lightmapIdx = 0;

    // Set up lightmap vector
    g_Lightmaps.resize(g_Faces.size());
    g_Lightmaps.shrink_to_fit();

    // Calculate stuff
    for (Face &face : g_Faces) {
        glm::vec2 lmFloatSize = face.planeMaxBounds / face.flPatchSize;

        face.lmSize.x = texFloatToInt(lmFloatSize.x);
        face.lmSize.y = texFloatToInt(lmFloatSize.y);

        pixelCount += (size_t)face.lmSize.x * face.lmSize.y;

        // Create lightmap
        g_Lightmaps[lightmapIdx] = LightmapTexture(face.lmSize);
        face.iLightmapIdx = (int)lightmapIdx;
        lightmapIdx++;
    }

    g_Patches.resize(pixelCount);
    s_PatchBounce.resize(pixelCount * (g_Config.iBounceCount + 1));
    logInfo("Total lightmap size: {:.3} megapixels ({:.3} MiB)", pixelCount / 1000000.0,
            pixelCount * sizeof(glm::vec3) / 1024.0 / 1024.0);
    logInfo("Memory used by patches: {:.3} MiB", pixelCount * sizeof(Patch) / 1024.0 / 1024.0);

    // Create patches
    size_t patchIdx = 0;
    for (Face &face : g_Faces) {
        LightmapTexture &lightmap = g_Lightmaps[face.iLightmapIdx];
        glm::ivec2 lmSize = face.lmSize;

        for (int y = 0; y < lmSize.y; y++) {
            for (int x = 0; x < lmSize.x; x++) {
                Patch &patch = g_Patches[patchIdx];
                patch.flSize = face.flPatchSize;

                glm::vec2 planePos = {face.planeMaxBounds.x * x / lmSize.x, face.planeMaxBounds.y * y / lmSize.y};
                planePos.x += patch.flSize * 0.5f;
                planePos.y += patch.flSize * 0.5f;

                patch.vOrigin = face.vPlaneOrigin + face.vI * planePos.x + face.vJ * planePos.y;
                patch.vNormal = face.vNormal;
                // patch.pPlane = face.pPlane;
                patch.pLMPixel = &lightmap.getPixel({x, y});

                // TODO: Remove that, read lights from a file
                if (face.iFlags & FACE_HACK) {
                    // Blue-ish color
                    patch.finalColor = getPatchBounce(patchIdx, 0) =
                        glm::vec3(194 / 255.f, 218 / 255.f, 252 / 255.f) * 100.f;
                }

                patchIdx++;
            }
        }
    }
    timer.stop();
    
    AFW_ASSERT(patchIdx == g_Patches.size());

    logInfo("        ... {:.3} s", timer.elapsedSeconds());
}

void calcViewFactors() {
    logInfo("Calculating view factors...");

    appfw::Timer timer;
    timer.start();

    size_t patchCount = g_Patches.size();

    for (size_t i = 0; i < patchCount; i++) {
        Patch &patch = g_Patches[i];
        float sum = 0;

        for (size_t j = 0; j < patchCount; j++) {
            if (i == j) {
                continue;
            }

            Patch &other = g_Patches[j];

            glm::vec3 dir = other.vOrigin - patch.vOrigin;
            float dist = glm::length(dir);

            if (floatEquals(dist, 0)) {
                continue;
            }

#if 0
            // FIXME: g_BSPTree.traceLine doesn't work properly.
            if (g_BSPTree.traceLine(patch.vOrigin, other.vOrigin)) {
                // Patches can't see each other
                continue;
            }
#endif

            dir = glm::normalize(dir);
            float cos1 = glm::dot(patch.vNormal, dir);
            float cos2 = -glm::dot(other.vNormal, dir);

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

            patch.viewFactors.insert({j, viewFactor});
            sum += viewFactor;
        }

        // Normalize view factors
        float k = 1 / sum;
        for (auto &j : patch.viewFactors) {
            j.second *= k;
        }
    }

    timer.stop();
    logInfo("        ... {:.3} s", timer.elapsedSeconds());
}

void bounceLight() {
    logInfo("Bouncing light...");

    size_t patchCount = g_Patches.size();

    for (size_t bounce = 1; bounce <= g_Config.iBounceCount; bounce++) {
        logInfo("Bounce {}", bounce);
        for (size_t i = 0; i < patchCount; i++) {
            Patch &patch = g_Patches[i];

            float p = 0.9f;
            glm::vec3 sum = {0, 0, 0};

            for (auto &j : patch.viewFactors) {
                sum += getPatchBounce(j.first, bounce - 1) * j.second;
            }

            getPatchBounce(i, bounce) = sum * p;
            g_Patches[i].finalColor += getPatchBounce(i, bounce);
        }
    }
}
