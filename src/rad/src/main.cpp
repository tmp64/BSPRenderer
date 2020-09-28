#include <appfw/binary_file.h>
#include <appfw/timer.h>
#include <appfw/init.h>
#include <appfw/services.h>
#include "main.h"
#include "utils.h"
#include "patches.h"
#include "bsp_tree.h"
#include "vis.h"
#include "plat.h"

std::string g_LevelPath;
bsp::Level g_Level;
RADConfig g_Config;

std::vector<Plane> g_Planes;
std::vector<Face> g_Faces;
std::vector<Patch> g_Patches;
std::vector<LightmapTexture> g_Lightmaps;

appfw::ThreadPool g_ThreadPool;
appfw::ThreadDispatcher g_Dispatcher;

void loadLevel(const std::string &levelPath);
void loadPlanes();
void loadFaces();

void applyColors();
void writeLightmaps();

int main(int, char **) {
    int returnCode = 0;
    appfw::init::init();

    try {
        appfw::Timer timer;
        timer.start();

        // TODO: Read config from cmd line args
        g_Config.flPatchSize = 32;
        g_Config.iBounceCount = 8;

        // Set up thread pool
        size_t threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) {
            logWarn("Failed to detect CPU thread count. Falling back to 1.");
            threadCount = 1;
        }

        g_ThreadPool.setThreadCount(threadCount);
        logInfo("Using {} thread(s).", threadCount);

        loadLevel("maps/crossfire.bsp");
        createPatches();
        buildVisMatrix();
        calcViewFactors();
        bounceLight();
        applyColors();
        writeLightmaps();

        timer.stop();
        logInfo("Done! Time taken: {:.3} s.", timer.elapsedSeconds());
    }
    catch (const std::exception &e) {
        logError("Exception in main():");
        logError("{}", e.what());
        returnCode = 1;
    }

    appfw::init::shutdown();
    return returnCode;
}

void loadLevel(const std::string &levelPath) {
    logInfo("Loading level from {}", levelPath);
    g_Level.loadFromFile(levelPath);
    g_LevelPath = levelPath;
    loadPlanes();
    loadFaces();
    g_BSPTree.createTree();
}

void loadPlanes() {
    logDebug("Loading planes");
    
    for (const bsp::BSPPlane &bspPlane : g_Level.getPlanes()) {
        Plane plane;

        // Copy BSPPlane
        plane.vNormal = glm::normalize(bspPlane.vNormal);
        plane.fDist = bspPlane.fDist;
        plane.nType = bspPlane.nType;

        // Calculate j
        glm::vec3 unprojJ;

        switch (plane.nType) {
        case bsp::PlaneType::PlaneX:
        case bsp::PlaneType::PlaneAnyX:
            unprojJ = {0, 0, 1};
            break;
        case bsp::PlaneType::PlaneY:
        case bsp::PlaneType::PlaneAnyY:
            unprojJ = {0, 0, 1};
            break;
        case bsp::PlaneType::PlaneZ:
        case bsp::PlaneType::PlaneAnyZ:
            unprojJ = {0, 1, 0};
            break;
        default:
            throw std::runtime_error("Map design error: invalid plane type");
        }

        plane.vJ = glm::normalize(projectVectorOnPlane(plane, unprojJ));

        g_Planes.push_back(plane);
    }

    g_Planes.shrink_to_fit();
}

void loadFaces() {
    logDebug("Loading faces");

    for (const bsp::BSPFace &bspFace : g_Level.getFaces()) {
        Face face;

        // Copy BSPFace
        face.iPlane = bspFace.iPlane;
        face.nPlaneSide = bspFace.nPlaneSide;
        face.iFirstEdge = bspFace.iFirstEdge;
        face.nEdges = bspFace.nEdges;
        face.iTextureInfo = bspFace.iTextureInfo;
        face.nLightmapOffset = bspFace.nLightmapOffset;
        memcpy(face.nStyles, bspFace.nStyles, sizeof(face.nStyles));

        // Face vars
        face.pPlane = &g_Planes.at(face.iPlane);
        
        if (face.nPlaneSide) {
            face.vNormal = -face.pPlane->vNormal;
        } else {
            face.vNormal = face.pPlane->vNormal;
        }

        face.vJ = face.pPlane->vJ;
        face.vI = glm::cross(face.vJ, face.vNormal);
        AFW_ASSERT(floatEquals(glm::length(face.vI), 1));
        AFW_ASSERT(floatEquals(glm::length(face.vJ), 1));

        // Vertices
        {
            auto verts = getFaceVerteces(g_Level, bspFace);

            for (glm::vec3 worldPos : verts) {
                Face::Vertex v;
                v.vWorldPos = worldPos;

                // Calculate plane coords
                v.vPlanePos.x = glm::dot(v.vWorldPos, face.vI);
                v.vPlanePos.y = glm::dot(v.vWorldPos, face.vJ);

                face.vertices.push_back(v);
            }

            face.vertices.shrink_to_fit();
        }

        // Normalize plane pos
        {
            float minX = 999999, minY = 999999;

            for (Face::Vertex &v : face.vertices) {
                minX = std::min(minX, v.vPlanePos.x);
                minY = std::min(minY, v.vPlanePos.y);
            }

            for (Face::Vertex &v : face.vertices) {
                v.vPlanePos.x -= minX;
                v.vPlanePos.y -= minY;
            }

            face.vPlaneOrigin = face.vertices[0].vWorldPos - face.vI * face.vertices[0].vPlanePos.x -
                                face.vJ * face.vertices[0].vPlanePos.y;
        }

        // Calculate plane bounds
        {
            float minX = 999999, minY = 999999;
            float maxX = -999999, maxY = -999999;

            for (Face::Vertex &v : face.vertices) {
                minX = std::min(minX, v.vPlanePos.x);
                minY = std::min(minY, v.vPlanePos.y);

                maxX = std::max(maxX, v.vPlanePos.x);
                maxY = std::max(maxY, v.vPlanePos.y);
            }

            face.planeMinBounds.x = minX;
            face.planeMinBounds.y = minY;

            face.planeMaxBounds.x = maxX;
            face.planeMaxBounds.y = maxY;

            AFW_ASSERT(floatEquals(minX, 0));
            AFW_ASSERT(floatEquals(minY, 0));

            // Also set lightmap coords
            float flTexMaxX = (float)texFloatToInt(maxX);
            float flTexMaxY = (float)texFloatToInt(maxY);
            for (Face::Vertex &v : face.vertices) {
                v.vLMCoord.x = v.vPlanePos.x / flTexMaxX;
                v.vLMCoord.y = v.vPlanePos.y / flTexMaxY;
            }
        }

        face.flPatchSize = g_Config.flPatchSize;

        // Check for sky
        {
            const bsp::BSPTextureInfo &texInfo = g_Level.getTexInfo().at(bspFace.iTextureInfo);
            const bsp::BSPMipTex &mipTex = g_Level.getTextures().at(texInfo.iMiptex);
            if (!strncmp(mipTex.szName, "SKY", 3) || !strncmp(mipTex.szName, "sky", 3)) {
                face.iFlags |= FACE_SKY;
            } else if (mipTex.szName[0] == '~') {
                // FIXME: Remove that later
                face.iFlags |= FACE_HACK;
            }
        }

        g_Faces.push_back(face);
    }

    g_Faces.shrink_to_fit();
}

/**
 * Applies color of patches to the lightmap.
 */
void applyColors() {
    logInfo("Applying colors...");
    
    for (Patch &p : g_Patches) {
        *p.pLMPixel = p.finalColor;
    }
}

/**
 * Writes mapname.bsp.lm file with all lightmaps.
 */
void writeLightmaps() {
    logInfo("Writing lightmaps...");
    
    struct LightmapFileHeader {
        uint32_t nMagic = ('1' << 24) | ('0' << 16) | ('M' << 8) | ('L' << 0);
        uint32_t iFaceCount = (uint32_t)g_Faces.size();
    };

    struct LightmapFaceInfo {
        uint32_t iVertexCount = 0;
        glm::ivec2 lmSize;
    };

    appfw::BinaryWriter file(g_LevelPath + ".lm");
    LightmapFileHeader lmHeader;
    file.write(lmHeader);

    AFW_ASSERT(g_Faces.size() == g_Lightmaps.size());

    // Write face info
    for (size_t i = 0; i < g_Faces.size(); i++) {
        const Face &face = g_Faces[i];
        LightmapFaceInfo info;
        info.iVertexCount = (uint32_t)face.vertices.size();
        info.lmSize = g_Lightmaps[i].size;
        file.write(info);

        // Write tex coords
        for (const Face::Vertex &v : face.vertices) {
            file.write(v.vLMCoord);
        }
    }

    // Write lightmaps
    for (LightmapTexture &lm : g_Lightmaps) {
        file.writeArray(appfw::span(lm.data));
    }
}
