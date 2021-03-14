#include <algorithm>
#include <fstream>
#include <appfw/binary_file.h>
#include <appfw/timer.h>
#include <rad/rad_sim.h>

namespace {

/**
 * Returns a point with coordinates (x, 0, 0) on a plane.
 */
inline glm::vec3 getXPointOnPlane(const bsp::BSPPlane &plane) { return {plane.fDist / plane.vNormal.x, 0.f, 0.f}; }

/**
 * Returns a point with coordinates (0, y, 0) on a plane.
 */
inline glm::vec3 getYPointOnPlane(const bsp::BSPPlane &plane) { return {0.f, plane.fDist / plane.vNormal.y, 0.f}; }

/**
 * Returns a point with coordinates (0, 0, z) on a plane.
 */
inline glm::vec3 getZPointOnPlane(const bsp::BSPPlane &plane) { return {0.f, 0.f, plane.fDist / plane.vNormal.z}; }

/**
 * Returns a projection of a point onto a plane.
 */
inline glm::vec3 projectPointOnPlane(const bsp::BSPPlane &plane, glm::vec3 point) {
    float h = glm::dot(plane.vNormal, point) - plane.fDist;
    return point - plane.vNormal * h;
}

/**
 * Returns a projection of a vector onto a plane. Length is not normalized.
 */
inline glm::vec3 projectVectorOnPlane(const bsp::BSPPlane &plane, glm::vec3 vec) {
    glm::vec3 a = projectPointOnPlane(plane, {0.f, 0.f, 0.f});
    glm::vec3 b = projectPointOnPlane(plane, vec);
    return b - a;
}

std::vector<glm::vec3> getFaceVertices(const bsp::Level &level, const bsp::BSPFace &face) {
    std::vector<glm::vec3> verts;
    auto &lvlVertices = level.getVertices();
    auto &lvlSurfEdges = level.getSurfEdges();

    for (int j = 0; j < face.nEdges; j++) {
        glm::vec3 vertex;
        bsp::BSPSurfEdge iEdgeIdx = lvlSurfEdges.at((size_t)face.iFirstEdge + j);

        if (iEdgeIdx > 0) {
            const bsp::BSPEdge &edge = level.getEdges().at(iEdgeIdx);
            vertex = lvlVertices.at(edge.iVertex[0]);
        } else {
            const bsp::BSPEdge &edge = level.getEdges().at(-iEdgeIdx);
            vertex = lvlVertices.at(edge.iVertex[1]);
        }

        verts.push_back(vertex);
    }

    return verts;
}

/**
 * Returns vector that points in the direction given in degrees (or -1/-2 for yaw).
 * Used for sunlight calculation.
 */
inline glm::vec3 getVecFromAngles(float pitch, float yaw) {
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

}

rad::RadSim::RadSim() : m_VisMat(this), m_VFList(this) {
    // Set up thread pool
    size_t threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) {
        logWarn("Failed to detect CPU thread count. Falling back to 1.");
        threadCount = 1;
    }

    m_ThreadPool.setThreadCount(threadCount);
    logInfo("Using {} thread(s).", threadCount);
}

void rad::RadSim::setLevel(const bsp::Level *pLevel, const std::string &path) {
    m_pLevel = pLevel;
    m_LevelPath = path;
    m_PatchHash = appfw::SHA256::Digest();
}

void rad::RadSim::loadLevelConfig() {
    m_PatchHash = appfw::SHA256::Digest();
    appfw::SHA256 hash;

    std::fstream configFile(getFileSystem().findFile(m_LevelPath + ".json", "assets"));
    configFile >> m_LevelConfigJson;

    // Load patch size
    m_flPatchSize = m_LevelConfigJson.at("base_patch_size").get<float>();
    m_PatchSizeStr = std::to_string(m_flPatchSize);
    std::replace(m_PatchSizeStr.begin(), m_PatchSizeStr.end(), '.', '_');

    m_flReflectivity = m_LevelConfigJson.at("base_reflectivity").get<float>();
    m_iBounceCount = m_LevelConfigJson.at("bounce_count").get<int>();

    // Load envlight
    if (m_LevelConfigJson.contains("envlight")) {
        const nlohmann::json &envlight = m_LevelConfigJson["envlight"];

        m_EnvLight.vColor.r = (float)envlight.at("color").at(0).get<int>();
        m_EnvLight.vColor.g = (float)envlight.at("color").at(1).get<int>();
        m_EnvLight.vColor.b = (float)envlight.at("color").at(2).get<int>();
        m_EnvLight.vColor *= envlight.at("brightness").get<float>();
        m_EnvLight.vDirection = getVecFromAngles(envlight.at("pitch"), envlight.at("yaw"));
    }

    loadPlanes();
    loadFaces();
    createPatches(hash);
    loadLevelEntities();

    m_PatchHash = hash.digest();
}

bool rad::RadSim::isVisMatValid() { return m_VisMat.isValid(); }

bool rad::RadSim::loadVisMat() {
    fs::path path = getFileSystem().findFileOrEmpty(getVisMatPath(), "assets");

    if (!path.empty()) {
        m_VisMat.loadFromFile(path);
        return m_VisMat.isValid();
    }

    return false;
}

void rad::RadSim::calcVisMat() {
    logInfo("Building visibility matrix...");
    appfw::Timer timer;
    timer.start();

    m_VisMat.buildVisMat();

    timer.stop();
    logInfo("Build vismat: {:.3} s", timer.elapsedSeconds());

    // Save vismat
    logInfo("Saving vismat...");
    fs::path path = getFileSystem().getFile(getVisMatPath(), "assets");
    m_VisMat.saveToFile(path);
}

bool rad::RadSim::isVFListValid() { return m_VFList.isValid(); }

bool rad::RadSim::loadVFList() {
    fs::path path = getFileSystem().findFileOrEmpty(getVFListPath(), "assets");

    if (!path.empty()) {
        m_VFList.loadFromFile(path);
        return m_VFList.isValid();
    }

    return false;
}

void rad::RadSim::calcViewFactors() {
    if (!isVisMatValid()) {
        throw std::logic_error("valid vismat required");
    }

    m_VFList.calculateVFList();

    // Save vflist
    logInfo("Saving VFList...");
    fs::path path = getFileSystem().getFile(getVFListPath(), "assets");
    m_VFList.saveToFile(path);
}

void rad::RadSim::bounceLight() {
    logInfo("Applying lighting...");
    clearBounceArray();
    addLighting();

    logInfo("Bouncing light...");
    float p = m_flReflectivity;
    PatchIndex patchCount = m_Patches.size();

    // Copy bounce 0 into the lightmap
    for (PatchIndex i = 0; i < patchCount; i++) {
        PatchRef patch(m_Patches, i);
        patch.getFinalColor() = getPatchBounce(i, 0);
    }

    // Bounce light
    for (int bounce = 1; bounce <= m_iBounceCount; bounce++) {
        updateProgress((bounce - 1.0) / (m_iBounceCount - 1.0));

        for (PatchIndex i = 0; i < patchCount; i++) {
            PatchRef patch(m_Patches, i);

            glm::vec3 sum = {0, 0, 0};

            for (auto &j : patch.getViewFactors().subspan(0, m_VFList.getPatchVFCount(i))) {
                sum += getPatchBounce(std::get<0>(j), bounce - 1) * std::get<1>(j);
            }

            getPatchBounce(i, bounce) = sum * p;
            patch.getFinalColor() += getPatchBounce(i, bounce);
        }
    }

    // Copy result into the lightmap
    for (PatchIndex i = 0; i < patchCount; i++) {
        PatchRef patch(m_Patches, i);
        *patch.getLMPixel() = patch.getFinalColor();
    }

    updateProgress(1);
}

void rad::RadSim::writeLightmaps() {
    logInfo("Writing lightmaps...");

    struct LightmapFileHeader {
        uint32_t nMagic = ('1' << 24) | ('0' << 16) | ('M' << 8) | ('L' << 0);
        uint32_t iFaceCount;
    };

    struct LightmapFaceInfo {
        uint32_t iVertexCount = 0;
        glm::ivec2 lmSize;
    };

    appfw::BinaryWriter file(getFileSystem().getFile(m_LevelPath + ".lm", "assets"));
    LightmapFileHeader lmHeader;
    lmHeader.iFaceCount = (uint32_t)m_Faces.size();
    file.write(lmHeader);

    AFW_ASSERT(m_Faces.size() == m_Lightmaps.size());

    // Write face info
    for (size_t i = 0; i < m_Faces.size(); i++) {
        const Face &face = m_Faces[i];
        LightmapFaceInfo info;
        info.iVertexCount = (uint32_t)face.vertices.size();
        info.lmSize = m_Lightmaps[i].size;
        file.write(info);

        // Write tex coords
        for (const Face::Vertex &v : face.vertices) {
            file.write(v.vLMCoord);
        }
    }

    // Write lightmaps
    for (LightmapTexture &lm : m_Lightmaps) {
        file.writeArray(appfw::span(lm.data));
    }
}

void rad::RadSim::addLighting() {
    applyEnvLight();
    applyTexLights();
}

void rad::RadSim::loadPlanes() {
    m_Planes.resize(m_pLevel->getPlanes().size());

    for (size_t i = 0; i < m_pLevel->getPlanes().size(); i++) {
        const bsp::BSPPlane &bspPlane = m_pLevel->getPlanes()[i];
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

        m_Planes[i] = plane;
    }
}

void rad::RadSim::loadFaces() {
    m_Faces.resize(m_pLevel->getFaces().size());

    for (size_t i = 0; i < m_pLevel->getFaces().size(); i++) {
        const bsp::BSPFace &bspFace = m_pLevel->getFaces()[i];
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
        face.pPlane = &m_Planes.at(face.iPlane);

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
            auto verts = getFaceVertices(*m_pLevel, bspFace);

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

        face.flPatchSize = m_flPatchSize;

        // Check for sky
        {
            const bsp::BSPTextureInfo &texInfo = m_pLevel->getTexInfo().at(bspFace.iTextureInfo);
            const bsp::BSPMipTex &mipTex = m_pLevel->getTextures().at(texInfo.iMiptex);

            if (!strncmp(mipTex.szName, "SKY", 3) || !strncmp(mipTex.szName, "sky", 3)) {
                face.iFlags |= FACE_SKY;
            }
        }

        m_Faces[i] = face;
    }
}

void rad::RadSim::createPatches(appfw::SHA256 &hash) {
    logInfo("Creating patches...");
    appfw::Timer timer;
    timer.start();
    size_t luxelCount = 0;
    size_t lightmapIdx = 0;

    // Set up lightmap vector
    m_Lightmaps.resize(m_Faces.size());
    m_Lightmaps.shrink_to_fit();

    // Calculate stuff
    for (Face &face : m_Faces) {
        glm::vec2 lmFloatSize = face.planeMaxBounds / face.flPatchSize;

        face.lmSize.x = texFloatToInt(lmFloatSize.x);
        face.lmSize.y = texFloatToInt(lmFloatSize.y);

        // Create lightmap
        m_Lightmaps[lightmapIdx] = LightmapTexture(face.lmSize);
        face.iLightmapIdx = (int)lightmapIdx;
        lightmapIdx++;

        // Skies don't have patches
        if (face.iFlags & FACE_SKY) {
            continue;
        }

        // Allocate patches
        face.iNumPatches = (size_t)face.lmSize.x * face.lmSize.y;
        luxelCount += face.iNumPatches;
    }

    if (luxelCount > MAX_PATCH_COUNT) {
        logError("Patch limit reached.");
        logError("Patch count: {}", luxelCount);
        logError("Patch limit: {}", MAX_PATCH_COUNT);
        throw std::runtime_error("patch limit reached");
    }

    m_Patches.allocate((PatchIndex)luxelCount);
    logInfo("Patch count: {}", luxelCount);
    logInfo("Total lightmap size: {:.3} megapixels ({:.3} MiB)", luxelCount / 1000000.0,
            luxelCount * sizeof(glm::vec3) / 1024.0 / 1024.0);
    logInfo("Memory used by patches: {:.3} MiB", luxelCount * m_Patches.getPatchMemoryUsage() / 1024.0 / 1024.0);

    {
        // Add patch count to hash
        PatchIndex patchCount = (PatchIndex)luxelCount;
        unsigned char patchCountBytes[sizeof(patchCount)];
        memcpy(patchCountBytes, &patchCount, sizeof(patchCount));
        hash.update(patchCountBytes, sizeof(patchCountBytes));
    }

    // Create patches
    PatchIndex patchIdx = 0;
    for (Face &face : m_Faces) {
        if (face.iFlags & FACE_SKY) {
            continue;
        }

        LightmapTexture &lightmap = m_Lightmaps[face.iLightmapIdx];
        glm::ivec2 lmSize = face.lmSize;

        AFW_ASSERT(face.iNumPatches > 0);
        face.iFirstPatch = patchIdx;

        for (int y = 0; y < lmSize.y; y++) {
            for (int x = 0; x < lmSize.x; x++) {
                PatchRef patch(m_Patches, patchIdx);
                patch.getSize() = face.flPatchSize;

                glm::vec2 planePos = {face.planeMaxBounds.x * x / lmSize.x, face.planeMaxBounds.y * y / lmSize.y};
                planePos.x += patch.getSize() * 0.5f;
                planePos.y += patch.getSize() * 0.5f;

                patch.getOrigin() = face.vPlaneOrigin + face.vI * planePos.x + face.vJ * planePos.y;
                patch.getNormal() = face.vNormal;
                patch.getPlane() = face.pPlane;
                patch.getLMPixel() = &lightmap.getPixel({x, y});

                // Add origin and plane to hash
                hash.update(reinterpret_cast<uint8_t *>(&patch.getOrigin()), sizeof(glm::vec3));
                hash.update(reinterpret_cast<uint8_t *>(&patch.getNormal()), sizeof(glm::vec3));

                patchIdx++;
            }
        }
    }
    timer.stop();

    AFW_ASSERT(patchIdx == m_Patches.size());

    logInfo("Create patches: {:.3} s", timer.elapsedSeconds());
}

void rad::RadSim::loadLevelEntities() {
    bool isEnvLightSet = m_EnvLight.vDirection.x != 0 || m_EnvLight.vDirection.y != 0 || m_EnvLight.vDirection.z != 0;
    bool wasEnvLightSet = isEnvLightSet;
    bool isEnvLightFound = false;

    for (auto &ent : m_pLevel->getEntities()) {
        std::string classname = ent.getValue<std::string>("classname", "");

        if (classname.empty()) {
            continue;
        }

        if (classname == "light") {

        } else if (classname == "light_environment") {
            if (!wasEnvLightSet) {
                if (!isEnvLightSet) {
                    isEnvLightSet = true;

                    // Read color
                    int color[4];
                    if (sscanf(ent.getValue<std::string>("_light").c_str(), "%d %d %d %d", &color[0], &color[1],
                               &color[2], &color[3]) != 4) {
                        throw std::runtime_error(fmt::format("light_environment _light = {} is invalid",
                                                             ent.getValue<std::string>("_light")));
                    }

                    m_EnvLight.vColor.r = color[0] / 255.f;
                    m_EnvLight.vColor.g = color[1] / 255.f;
                    m_EnvLight.vColor.b = color[2] / 255.f;
                    m_EnvLight.vColor *= color[3];
                    m_EnvLight.vColor /= m_pAppConfig->getItem("rad").get<float>("light_env_brightness_divisor");

                    // Read angle
                    float yaw = ent.getValue<float>("angle");
                    float pitch = ent.getValue<float>("pitch");
                    m_EnvLight.vDirection = getVecFromAngles(pitch, yaw);
                }

                if (isEnvLightFound) {
                    logWarn("Level has multiple light_environment. Only first one is used.");
                }

                isEnvLightFound = true;
            }
        }
    }
}

glm::vec3 &rad::RadSim::getPatchBounce(size_t patch, size_t bounce) {
    AFW_ASSERT(patch < m_Patches.size() && bounce <= m_iBounceCount);
    return m_PatchBounce[patch * (m_iBounceCount + 1) + bounce];
}

void rad::RadSim::clearBounceArray() {
    m_PatchBounce.resize((size_t)m_Patches.size() * (m_iBounceCount + 1));
    std::fill(m_PatchBounce.begin(), m_PatchBounce.end(), glm::vec3(0, 0, 0));
}

void rad::RadSim::applyEnvLight() {
    bool isEnvLightSet = m_EnvLight.vDirection.x != 0 || m_EnvLight.vDirection.y != 0 || m_EnvLight.vDirection.z != 0;

    if (!isEnvLightSet) {
        return;
    }

    for (PatchIndex patchIdx = 0; patchIdx < m_Patches.size(); patchIdx++) {
        PatchRef patch(m_Patches, patchIdx);

        // Check if patch can be hit by the sky
        float cosangle = -glm::dot(patch.getNormal(), m_EnvLight.vDirection);

        if (cosangle < 0.001f) {
            continue;
        }

        // Cast a ray to the sky and check if it hits
        glm::vec3 from = patch.getOrigin();
        glm::vec3 to = from + (m_EnvLight.vDirection * -SKY_RAY_LENGTH);

        if (m_pLevel->traceLine(from, to) == bsp::CONTENTS_SKY) {
            // Hit the sky, add the sun color
            getPatchBounce(patchIdx, 0) += m_EnvLight.vColor * cosangle;
        }
    }
}

void rad::RadSim::applyTexLights() {
    for (Face &face : m_Faces) {
        const bsp::BSPTextureInfo &texinfo = m_pLevel->getTexInfo()[face.iTextureInfo];
        const bsp::BSPMipTex &miptex = m_pLevel->getTextures()[texinfo.iMiptex];

        // TODO: Actual texinfo
        if (miptex.szName[0] == '~') {
            for (PatchIndex p = face.iFirstPatch; p < face.iFirstPatch + face.iNumPatches; p++) {
                getPatchBounce(p, 0) += glm::vec3(1, 1, 1) * 50.f;
            }
        }
    }
}

void rad::RadSim::updateProgress(double progress) {
    if (m_fnProgressCallback) {
        m_fnProgressCallback(progress);
    }
}

std::string rad::RadSim::getVisMatPath() {
    return fmt::format("{}.vismat{}", m_LevelPath, m_PatchSizeStr);
}

std::string rad::RadSim::getVFListPath() {
    return fmt::format("{}.vflist{}", m_LevelPath, m_PatchSizeStr);
}
