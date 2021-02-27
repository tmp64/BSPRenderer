#include <nlohmann/json.hpp>
#include <appfw/binary_file.h>
#include <appfw/timer.h>
#include <appfw/init.h>
#include <appfw/services.h>
#include <app_base/app_config.h>
#include "main.h"
#include "utils.h"
#include "patches.h"
#include "vis.h"
#include "plat.h"
#include "patch_list.h"

bsp::Level g_Level;
RADConfig g_Config;
AppConfig g_AppConfig;

EnvLight g_EnvLight;

std::vector<Plane> g_Planes;
std::vector<Face> g_Faces;
std::vector<LightmapTexture> g_Lightmaps;

appfw::ThreadPool g_ThreadPool;
appfw::ThreadDispatcher g_Dispatcher;

void initApp();
void loadLevel();
void loadPlanes();
void loadFaces();
void loadEnvLight();

void applyColors();
void writeLightmaps();

int main(int, char **) {
    int returnCode = 0;
    appfw::init::init();

    conPrintStrong("---------------------------------------------------");
    conPrintStrong("Radiosity simulator for Half-Life BSP Renderer");
    conPrintStrong("https://github.com/tmp64/BSPRenderer");
    conPrintStrong("---------------------------------------------------");

    try {
        appfw::Timer timer;
        timer.start();

        initApp();

        // Set up thread pool
        size_t threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) {
            logWarn("Failed to detect CPU thread count. Falling back to 1.");
            threadCount = 1;
        }

        g_ThreadPool.setThreadCount(threadCount);
        logInfo("Using {} thread(s).", threadCount);

        loadLevel();
        createPatches();
        buildVisMatrix();
        calcViewFactors();
        applyEnvLighting();
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

void initApp() {
    // Init file system
    fs::path baseAppPath = fs::current_path();
    logInfo("Base app path: {}", baseAppPath.u8string());
    getFileSystem().addSearchPath(baseAppPath, "base");

    // Load app config
    g_AppConfig.loadJsonFile(getFileSystem().findFile("bspviewer/app_config.json", "base"));
    g_AppConfig.mountFilesystem();

    // Parse app config
    g_Config.flPatchSize = g_AppConfig.getItem("rad").get<float>("patch_size");
    g_Config.iBounceCount = g_AppConfig.getItem("rad").get<int>("bounce_count");
    g_Config.flReflectivity = g_AppConfig.getItem("rad").get<float>("reflectivity");

    nlohmann::json lightColor = g_AppConfig.getItem("rad").getSubItem("light_color").getJson();
    g_Config.flThatLight.r = lightColor.at(0) / 255.f;
    g_Config.flThatLight.g = lightColor.at(1) / 255.f;
    g_Config.flThatLight.b = lightColor.at(2) / 255.f;
    g_Config.flThatLight *= g_AppConfig.getItem("rad").get<float>("light_brightness");

    g_Config.mapName = g_AppConfig.getItem("rad").get<std::string>("map");

    conPrint("");
    conPrintStrong("Configuration:");
    conPrint("Patch size: {} units", g_Config.flPatchSize);
    conPrint("Bounce count: {}", g_Config.iBounceCount);
    conPrint("Surface reflectivity: {}", g_Config.flReflectivity);
    conPrint("Light brightness: [{}, {}, {}]", g_Config.flThatLight.r, g_Config.flThatLight.g, g_Config.flThatLight.b);
    conPrint("Map: {}", g_Config.mapName);
    conPrint("");
}

void loadLevel() {
    fs::path levelPath = getFileSystem().findFile("maps/" + g_Config.mapName + ".bsp", "assets");
    g_Level.loadFromFile(levelPath);
    loadPlanes();
    loadFaces();
    loadEnvLight();
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
            }
            else if (mipTex.szName[0] == '~') {
                // FIXME: Remove that later
                face.iFlags |= FACE_HACK;
            }
        }

        g_Faces.push_back(face);
    }

    g_Faces.shrink_to_fit();
}

void loadEnvLight() {
    const bsp::Level::EntityListItem *pLight = g_Level.getEntities().findEntityByClassname("light_environment");

    if (!pLight) {
        return;
    }

    // Read color
    int color[4];
    if (sscanf(pLight->getValue<std::string>("_light").c_str(), "%d %d %d %d", &color[0], &color[1], &color[2],
        &color[3]) != 4) {
        throw std::runtime_error(
            fmt::format("light_environment _light = {} is invalid", pLight->getValue<std::string>("_light")));
    }

    g_EnvLight.vColor.r = color[0] / 255.f;
    g_EnvLight.vColor.g = color[1] / 255.f;
    g_EnvLight.vColor.b = color[2] / 255.f;
    g_EnvLight.vColor *= color[3];
    g_EnvLight.vColor /= g_AppConfig.getItem("rad").get<float>("light_env_brightness_divisor");

    // Read yaw
    float yaw = pLight->getValue<float>("angle");

    if (yaw == -1) {
        // ANGLE_UP
        g_EnvLight.vDirection.x = 0;
        g_EnvLight.vDirection.y = 0;
        g_EnvLight.vDirection.z = 1;
    }
    else if (yaw == -2) {
        // ANGLE_DOWN
        g_EnvLight.vDirection.x = 0;
        g_EnvLight.vDirection.y = 0;
        g_EnvLight.vDirection.z = -1;
    }
    else {
        g_EnvLight.vDirection.x = cos(glm::radians(yaw));
        g_EnvLight.vDirection.y = sin(glm::radians(yaw));
        g_EnvLight.vDirection.z = 0;
    }

    // Read pitch
    float pitch = pLight->getValue<float>("pitch");
    g_EnvLight.vDirection.x *= cos(glm::radians(pitch));
    g_EnvLight.vDirection.y *= cos(glm::radians(pitch));
    g_EnvLight.vDirection.z = sin(glm::radians(pitch));

    g_EnvLight.vDirection = glm::normalize(g_EnvLight.vDirection);

    logInfo("Envlight direction: [{}, {}, {}]", g_EnvLight.vDirection.x, g_EnvLight.vDirection.y,
            g_EnvLight.vDirection.z);

    // Check if there are multiple suns
    if (g_Level.getEntities().findEntityByClassname("light_environment", pLight)) {
        logWarn("Level has multiple light_environment. Only first one is used.");
    }
}

/**
 * Applies color of patches to the lightmap.
 */
void applyColors() {
    logInfo("Applying colors...");
    
    for (PatchRef p : g_Patches) {
        *p.getLMPixel() = p.getFinalColor();
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

    appfw::BinaryWriter file(getFileSystem().getFile("maps/" + g_Config.mapName + ".bsp.lm", "assets"));
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
