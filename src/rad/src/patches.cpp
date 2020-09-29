#include <appfw/binary_file.h>
#include <appfw/timer.h>
#include <appfw/services.h>
#include "patches.h"
#include "main.h"
#include "utils.h"
#include "bsp_tree.h"
#include "vis.h"
#include "patch_list.h"

static std::vector<glm::vec3> s_PatchBounce;
static std::vector<ViewFactor> s_ViewFactors;
static std::vector<size_t> s_ViewFactorCount;

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

        // Create lightmap
        g_Lightmaps[lightmapIdx] = LightmapTexture(face.lmSize);
        face.iLightmapIdx = (int)lightmapIdx;
        lightmapIdx++;

        // Skies don't have patches
        if (face.iFlags & FACE_SKY) {
            continue;
        }

        // Allocate patches
        face.iNumPatches = (size_t)face.lmSize.x * face.lmSize.y;
        pixelCount += face.iNumPatches;
    }

    g_Patches.allocate(pixelCount);
    s_PatchBounce.resize(pixelCount * (g_Config.iBounceCount + 1));
    logInfo("Patch count: {}", pixelCount);
    logInfo("Total lightmap size: {:.3} megapixels ({:.3} MiB)", pixelCount / 1000000.0,
            pixelCount * sizeof(glm::vec3) / 1024.0 / 1024.0);
    logInfo("Memory used by patches: {:.3} MiB", pixelCount * g_Patches.getPatchMemoryUsage() / 1024.0 / 1024.0);

    // Create patches
    size_t patchIdx = 0;
    for (Face &face : g_Faces) {
        if (face.iFlags & FACE_SKY) {
            continue;
        }

        LightmapTexture &lightmap = g_Lightmaps[face.iLightmapIdx];
        glm::ivec2 lmSize = face.lmSize;

        AFW_ASSERT(face.iNumPatches > 0);
        face.iFirstPatch = patchIdx;

        for (int y = 0; y < lmSize.y; y++) {
            for (int x = 0; x < lmSize.x; x++) {
                PatchRef patch(patchIdx);
                patch.getSize() = face.flPatchSize;

                glm::vec2 planePos = {face.planeMaxBounds.x * x / lmSize.x, face.planeMaxBounds.y * y / lmSize.y};
                planePos.x += patch.getSize() * 0.5f;
                planePos.y += patch.getSize() * 0.5f;

                patch.getOrigin() = face.vPlaneOrigin + face.vI * planePos.x + face.vJ * planePos.y;
                patch.getNormal() = face.vNormal;
                patch.getPlane() = face.pPlane;
                patch.getLMPixel() = &lightmap.getPixel({x, y});

                // TODO: Remove that, read lights from a file
                if (face.iFlags & FACE_HACK) {
                    // Blue-ish color
                    patch.getFinalColor() = getPatchBounce(patchIdx, 0) =
                        glm::vec3(194 / 255.f, 218 / 255.f, 252 / 255.f) * 10.f;
                }

                patchIdx++;
            }
        }
    }
    timer.stop();
    
    AFW_ASSERT(patchIdx == g_Patches.size());

    logInfo("        ... {:.3} s", timer.elapsedSeconds());
}

void viewFactorWorker(appfw::ThreadPool::ThreadInfo &ti) {
    size_t patchCount = g_Patches.size();

    for (size_t i = g_Dispatcher.getWork(); i != g_Dispatcher.WORK_DONE; i = g_Dispatcher.getWork(), ti.iWorkDone++) {
        PatchRef patch(i);
        float sum = 0;
        size_t viewFactorIdx = 0;

        for (size_t j = 0; j < patchCount; j++) {
            if (i == j) {
                continue;
            }

            PatchRef other(j);

            glm::vec3 dir = other.getOrigin() - patch.getOrigin();
            float dist = glm::length(dir);

            if (floatEquals(dist, 0)) {
                continue;
            }

            /*if (g_BSPTree.traceLine(patch.vOrigin, other.vOrigin)) {
                // Patches can't see each other
                continue;
            }*/

            if (!checkVisBit(i, j)) {
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

        s_ViewFactorCount[i] = viewFactorIdx;

        // Normalize view factors
        float k = 1 / sum;
        for (size_t j = 0; j < viewFactorIdx; j++) {
            std::get<1>(patch.getViewFactors()[j]) *= k;
        }
    }
}

void calcViewFactors() {
    logInfo("Gathering data for view factors...");
    {
        appfw::Timer timer;
        timer.start();

        size_t totalCount = 0;
        s_ViewFactorCount.resize(g_Patches.size());

        for (size_t i = 0; i < g_Patches.size(); i++) {
            for (size_t j = i + 1; j < g_Patches.size(); j++) {
                if (checkVisBit(i, j)) {
                    s_ViewFactorCount[i] += 1;
                    s_ViewFactorCount[j] += 1;
                    totalCount += 2;
                }
            }
        }

        timer.stop();
        logInfo("        ... {:.3} s", timer.elapsedSeconds());
        logInfo("Memory required for viewfactor list: {:.3f} MiB", sizeof(ViewFactor) * totalCount / 1024.0 / 1024.0);

        logInfo("Allocating memory...");
        s_ViewFactors.resize(totalCount);
        appfw::span<ViewFactor> span(s_ViewFactors);

        logInfo("Writing pointers into patches...");
        size_t nextIdx = 0;
        for (size_t i = 0; i < g_Patches.size(); i++) {
            PatchRef patch(i);
            patch.getViewFactors() = span.subspan(nextIdx, s_ViewFactorCount[i]);
            nextIdx += s_ViewFactorCount[i];
        }
    }

    logInfo("Calculating view factors...");

    appfw::Timer timer;
    timer.start();

    g_Dispatcher.setWorkCount(g_Patches.size());
    g_ThreadPool.run(viewFactorWorker, 0);

    double lastPercentageShown = 0;

    while (!g_ThreadPool.isFinished()) {
        double time = timer.elapsedSeconds();
        if (time > lastPercentageShown + 2) {
            double done = (double)g_ThreadPool.getStatus() / g_Patches.size();
            printf("%.f%%...", done * 100.0);
            lastPercentageShown = time;
        }

        std::this_thread::sleep_for(g_ThreadPool.POLL_DELAY);
    }

    printf("\n");

    if (g_ThreadPool.getStatus() != g_Patches.size()) {
        logError("Threading failure!");
        abort();
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
            PatchRef patch(i);

            float p = 0.9f;
            glm::vec3 sum = {0, 0, 0};

            for (auto &j : patch.getViewFactors().subspan(0, s_ViewFactorCount[i])) {
                sum += getPatchBounce(std::get<0>(j), bounce - 1) * std::get<1>(j);
            }

            getPatchBounce(i, bounce) = sum * p;
            patch.getFinalColor() += getPatchBounce(i, bounce);
        }
    }
}
