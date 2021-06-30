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

rad::RadSim::RadSim()
    : m_VisMat(this)
    , m_VFList(this)
    , m_SVisMat(this) {
    printi("Using {} thread(s).", m_Executor.num_workers());
}

void rad::RadSim::setLevel(const bsp::Level *pLevel, const std::string &path) {
    m_pLevel = pLevel;
    m_LevelPath = path;
    m_PatchHash = appfw::SHA256::Digest();
}

void rad::RadSim::loadLevelConfig() {
    m_PatchHash = appfw::SHA256::Digest();
    appfw::SHA256 hash;

    std::fstream configFile(getFileSystem().findExistingFile(m_LevelPath + ".json"));
    configFile >> m_LevelConfigJson;

    // Load patch size
    m_flPatchSize = m_LevelConfigJson.at("base_patch_size").get<float>();
    m_flLuxelSize = m_flPatchSize; // FIXME:
    m_PatchSizeStr = std::to_string(m_flPatchSize);
    std::replace(m_PatchSizeStr.begin(), m_PatchSizeStr.end(), '.', '_');

    m_flReflectivity = m_LevelConfigJson.at("base_reflectivity").get<float>();
    m_iBounceCount = m_LevelConfigJson.at("bounce_count").get<int>();
    m_flGamma = m_LevelConfigJson.at("gamma").get<float>();

    // Load envlight
    if (m_LevelConfigJson.contains("envlight")) {
        const nlohmann::json &envlight = m_LevelConfigJson["envlight"];

        m_EnvLight.vColor.r = envlight.at("color").at(0).get<int>() / 255.f;
        m_EnvLight.vColor.g = envlight.at("color").at(1).get<int>() / 255.f;
        m_EnvLight.vColor.b = envlight.at("color").at(2).get<int>() / 255.f;
        m_EnvLight.vColor = correctColorGamma(m_EnvLight.vColor);
        m_EnvLight.vColor *= envlight.at("brightness").get<float>();
        m_EnvLight.vDirection = getVecFromAngles(envlight.at("pitch"), envlight.at("yaw"));
    }

    loadPlanes();
    loadFaces();
    createPatches(hash);
    loadLevelEntities();

    m_PatchHash = hash.digest();
}

bool rad::RadSim::isVisMatValid() { return m_SVisMat.isValid(); }

bool rad::RadSim::loadVisMat() {
    fs::path path = getFileSystem().findExistingFile(getVisMatPath(), std::nothrow);

    if (!path.empty()) {
        m_SVisMat.loadFromFile(path);
        return m_SVisMat.isValid();
    }

    return false;
}

void rad::RadSim::calcVisMat() {
    appfw::Timer timer;

    // Build full vismat
    printi("Building visibility matrix...");
    timer.start();
    m_VisMat.buildVisMat();
    timer.stop();
    printi("Build vismat: {:.3} s", timer.dseconds());

    // Build sparse vismat
    printi("Building sparse vismat...");
    timer.start();
    m_SVisMat.buildSparseMat();
    timer.stop();
    printi("Build sparse vismat: {:.3} s", timer.dseconds());

    // Clear normal vismat
    m_VisMat.unloadVisMat();

    // Save vismat
    printi("Saving svismat...");
    fs::path path = getFileSystem().getFilePath(getVisMatPath());
    m_SVisMat.saveToFile(path);
}

bool rad::RadSim::isVFListValid() { return m_VFList.isValid(); }

bool rad::RadSim::loadVFList() {
    fs::path path = getFileSystem().findExistingFile(getVFListPath(), std::nothrow);

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
    /*printi("Saving VFList...");
    fs::path path = getFileSystem().getFile(getVFListPath(), "assets");
    m_VFList.saveToFile(path);*/
}

void rad::RadSim::bounceLight() {
    printi("Applying lighting...");
    clearBounceArray();
    addLighting();

    
    printi("Bouncing light...");
    appfw::Timer timer;
    timer.start();
    float refl = m_flReflectivity;
    PatchIndex patchCount = m_Patches.size();

    // Copy bounce 0 into the lightmap
    for (PatchIndex i = 0; i < patchCount; i++) {
        PatchRef patch(m_Patches, i);
        patch.getFinalColor() = getPatchBounce(i, 0);
    }
    
    std::vector<glm::vec3> patchSum(m_Patches.size()); //!< Sum of all light for each patch

    // Bounce light
    for (int bounce = 1; bounce <= m_iBounceCount; bounce++) {
        updateProgress((bounce - 1.0) / (m_iBounceCount - 1.0));

        std::fill(patchSum.begin(), patchSum.end(), glm::vec3(0, 0, 0));

        for (PatchIndex i = 0; i < patchCount; i++) {
            // Bounce light for each visible path
            receiveLight(bounce, i, patchSum);
        }

        for (PatchIndex i = 0; i < patchCount; i++) {
            // Received light from all visible patches
            // Write it
            PatchRef patch(m_Patches, i);
            AFW_ASSERT(patchSum[i].r >= 0 && patchSum[i].g >= 0 && patchSum[i].b >= 0);
            getPatchBounce(i, bounce) = patchSum[i] * refl;
            patch.getFinalColor() += getPatchBounce(i, bounce);
        }
    }

    updateProgress(1);

    timer.stop();
    printi("Bounce light: {:.3} s", timer.dseconds());
}

void rad::RadSim::writeLightmaps() {
    printn("Sampling lightmaps...");
    appfw::Timer timer;
    timer.start();

    for (size_t i = 0; i < m_Faces.size(); i++) {
        Face &face = m_Faces[i];
        
        if (!face.hasLightmap()) {
            continue;
        }

        sampleLightmap(i);
    }

    timer.stop();
    printi("Sample lightmaps: {:.3} s", timer.dseconds());

    //-------------------------------------------------------------------------
    printn("Writing lightmaps...");

    struct LightmapFileHeader {
        uint32_t nMagic = ('1' << 24) | ('0' << 16) | ('M' << 8) | ('L' << 0);
        uint32_t iFaceCount;
    };

    struct LightmapFaceInfo {
        uint32_t iVertexCount = 0;
        glm::ivec2 lmSize;
    };

    appfw::BinaryOutputFile file(getFileSystem().getFilePath(m_LevelPath + ".lm"));
    LightmapFileHeader lmHeader;
    lmHeader.iFaceCount = (uint32_t)m_Faces.size();
    file.writeObject(lmHeader);

    AFW_ASSERT(m_Faces.size() == m_Lightmaps.size());

    // Write face info
    for (size_t i = 0; i < m_Faces.size(); i++) {
        Face &face = m_Faces[i];
        LightmapFaceInfo info;
        info.iVertexCount = (uint32_t)face.vertices.size();

        if (face.hasLightmap()) {
            AFW_ASSERT(face.iLightmapIdx != -1);
            info.lmSize = m_Lightmaps[i].size;
        } else {
            info.lmSize = {0, 0};
            AFW_ASSERT(m_Lightmaps[i].size == info.lmSize);
        }
        
        file.writeObject(info);

        // Write tex coords
        for (const Face::Vertex &v : face.vertices) {
            file.writeFloat(v.vLMCoord.x);
            file.writeFloat(v.vLMCoord.y);
        }
    }

    // Write lightmaps
    for (LightmapTexture &lm : m_Lightmaps) {
        if (lm.size.x != 0) {
#if 1
            file.writeObjectArray(appfw::span(lm.data));
#else
            // Write random pixels
            for (size_t i = 0; i < lm.data.size(); i++) {
                int rand0 = rand() % 255;
                int rand1 = rand() % 255;
                int rand2 = rand() % 255;
                file.write(rand0 / 255.f);
                file.write(rand1 / 255.f);
                file.write(rand2 / 255.f);
            }
#endif
        }
    }
}

void rad::RadSim::addLighting() {
    applyEnvLight();
    applyTexLights();
}

float rad::RadSim::getMinPatchSize() {
    // TODO: Read from config
    return 4.0f;
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

    size_t lightmapCount = 0;

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

        // Calculate plane bounds and center
        {
            float minX = 999999, minY = 999999;
            float maxX = -999999, maxY = -999999;
            face.planeCenterPoint = glm::vec2(0, 0);

            for (Face::Vertex &v : face.vertices) {
                minX = std::min(minX, v.vPlanePos.x);
                minY = std::min(minY, v.vPlanePos.y);

                maxX = std::max(maxX, v.vPlanePos.x);
                maxY = std::max(maxY, v.vPlanePos.y);

                face.planeCenterPoint += v.vPlanePos;
            }

            face.planeCenterPoint /= (float)face.vertices.size();

            face.planeMinBounds.x = minX;
            face.planeMinBounds.y = minY;

            face.planeMaxBounds.x = maxX;
            face.planeMaxBounds.y = maxY;

            AFW_ASSERT(floatEquals(minX, 0));
            AFW_ASSERT(floatEquals(minY, 0));
        }

        face.flPatchSize = m_flPatchSize;

        // Lightmap size and coords
        {
            glm::vec2 baseGridSizeFloat = face.planeMaxBounds / m_flLuxelSize;
            face.vLightmapSize.x = texFloatToInt(baseGridSizeFloat.x);
            face.vLightmapSize.y = texFloatToInt(baseGridSizeFloat.y);

            for (Face::Vertex &v : face.vertices) {
                v.vLMCoord.x = v.vPlanePos.x / (m_flLuxelSize * face.vLightmapSize.x);
                v.vLMCoord.y = v.vPlanePos.y / (m_flLuxelSize * face.vLightmapSize.y);
            }
        }

        // Check for sky
        {
            const bsp::BSPTextureInfo &texInfo = m_pLevel->getTexInfo().at(bspFace.iTextureInfo);
            const bsp::BSPMipTex &mipTex = m_pLevel->getTextures().at(texInfo.iMiptex);

            if (!strncmp(mipTex.szName, "SKY", 3) || !strncmp(mipTex.szName, "sky", 3)) {
                face.iFlags |= FACE_SKY;
            }
        }

        m_Faces[i] = face;

        if (face.hasLightmap()) {
            lightmapCount++;
        }
    }

    m_Lightmaps.resize(m_Faces.size()); // must have same size as faces!
    printi("{} faces have lightmaps", lightmapCount);
}

void rad::RadSim::createPatches(appfw::SHA256 &hash) {
    printn("Creating patches...");
    appfw::Timer timer;
    timer.start();
    m_PatchTrees.resize(m_Faces.size());

    std::atomic<PatchIndex> totalPatchCount;
    int lightmapIdx = 0;
    size_t luxelCount = 0;

    AFW_ASSERT(m_Lightmaps.size() == m_Faces.size());

    for (size_t i = 0; i < m_Faces.size(); i++) {
        Face &face = m_Faces[i];

        // Allocate a lightmap for the face (even if it doesn't have one)
        face.iLightmapIdx = lightmapIdx;
        lightmapIdx++;

        if (!face.hasLightmap()) {
            continue;
        }

        // Allocate memory for a lightmap
        m_Lightmaps[face.iLightmapIdx] = LightmapTexture(face.vLightmapSize);
        luxelCount += (size_t)face.vLightmapSize.x * face.vLightmapSize.y;

        // This can be running in multiple threads
        // (but it takes less than a second so it's fine)
        PatchTree &tree = m_PatchTrees[i];
        tree.createPatches(this, face, totalPatchCount, getMinPatchSize());
    }

    //-------------------------------------------------------------------------
    PatchIndex patchCount = totalPatchCount.load();
    m_Patches.allocate(patchCount);
    printi("Patch count: {}", patchCount);
    printi("Total lightmap size: {:.3} megapixels ({:.3} MiB)", luxelCount / 1000000.0,
           luxelCount * sizeof(glm::vec3) / 1024.0 / 1024.0);
    printi("Memory used by patches: {:.3} MiB",
           patchCount * m_Patches.getPatchMemoryUsage() / 1024.0 / 1024.0);

    if (patchCount == 0) {
        throw std::runtime_error("No patches were created. Bug or empty map?");
    }

   {
        // Add patch count to hash
        uint8_t patchCountBytes[sizeof(patchCount)];
        memcpy(patchCountBytes, &patchCount, sizeof(patchCount));
        hash.update(patchCountBytes, sizeof(patchCountBytes));
    }

    //-------------------------------------------------------------------------
    printn("Transferring patches into the list");

    PatchIndex offset = 0;
    for (size_t i = 0; i < m_Faces.size(); i++) {
        Face &face = m_Faces[i];

        if (!face.hasLightmap()) {
            continue;
        }

        face.iFirstPatch = offset;
        face.iNumPatches = m_PatchTrees[i].copyPatchesToTheList(offset, hash);
        offset += face.iNumPatches;
    }

    AFW_ASSERT(offset == m_Patches.size());
    
    
    //-------------------------------------------------------------------------
    printn("Building quadtrees");

    for (size_t i = 0; i < m_Faces.size(); i++) {
        Face &face = m_Faces[i];

        if (!face.hasLightmap()) {
            continue;
        }

        m_PatchTrees[i].buildTree();
    }

    timer.stop();
    printi("Create patches: {:.3} s", timer.dseconds());

    //-------------------------------------------------------------------------
    std::string patchDumpFile = getCommandLine().getArgString("--dump-patches", "");
    if (!patchDumpFile.empty()) {
        printn("Dumping patches to {}", patchDumpFile);
        appfw::BinaryOutputFile file(getFileSystem().getFilePath("assets:" + patchDumpFile));

        #if 0
        file.writeUInt32(m_Faces[0].vertices.size());

        for (auto &i : m_Faces[0].vertices) {
            file.writeFloat(i.vWorldPos.x);
            file.writeFloat(i.vWorldPos.y);
            file.writeFloat(i.vWorldPos.z);
        }

        #else

        file.writeUInt32(patchCount * 8);

        for (size_t i = 0; i < m_Faces.size(); i++) {
            Face &face = m_Faces[i];

            if (!face.hasLightmap()) {
                continue;
            }

            
            for (PatchIndex j = 0; j < face.iNumPatches; j++) {
                PatchRef patch(m_Patches, face.iFirstPatch + j);
                
                auto fnWriteVec = [&file](glm::vec3 v) {
                    file.writeFloat(v.x);
                    file.writeFloat(v.y);
                    file.writeFloat(v.z);
                };

                float k = patch.getSize() / 2.0f;
                glm::vec3 corners[4];
                #if 1
                // Lines
                corners[0] = patch.getOrigin() - face.vI * k - face.vJ * k;
                corners[1] = patch.getOrigin() + face.vI * k - face.vJ * k;
                corners[2] = patch.getOrigin() + face.vI * k + face.vJ * k;
                corners[3] = patch.getOrigin() - face.vI * k + face.vJ * k;
                #else
                // Points
                corners[0] = patch.getOrigin();
                corners[1] = patch.getOrigin();
                corners[2] = patch.getOrigin();
                corners[3] = patch.getOrigin();
                #endif

                fnWriteVec(corners[0]);
                fnWriteVec(corners[1]);

                fnWriteVec(corners[1]);
                fnWriteVec(corners[2]);

                fnWriteVec(corners[2]);
                fnWriteVec(corners[3]);

                fnWriteVec(corners[3]);
                fnWriteVec(corners[0]);
            }
        }
        #endif

        printi("Dumped");
    }
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
                    m_EnvLight.vColor = correctColorGamma(m_EnvLight.vColor);
                    m_EnvLight.vColor *= color[3];
                    m_EnvLight.vColor /= m_pAppConfig->getItem("rad").get<float>("light_env_brightness_divisor");

                    // Read angle
                    float yaw = ent.getValue<float>("angle");
                    float pitch = ent.getValue<float>("pitch");
                    m_EnvLight.vDirection = getVecFromAngles(pitch, yaw);
                }

                if (isEnvLightFound) {
                    printw("Level has multiple light_environment. Only first one is used.");
                }

                isEnvLightFound = true;
            }
        }
    }
}

glm::vec3 rad::RadSim::correctColorGamma(const glm::vec3 &color) {
    glm::vec3 c = color;
    c.r = std::pow(c.r, m_flGamma);
    c.g = std::pow(c.g, m_flGamma);
    c.b = std::pow(c.b, m_flGamma);
    return c;
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

void rad::RadSim::receiveLight(int bounce, PatchIndex i, std::vector<glm::vec3> &patchSum) {
    auto &countTable = m_SVisMat.getCountTable();
    auto &offsetTable = m_SVisMat.getOffsetTable();
    auto &listItems = m_SVisMat.getListItems();
    auto &vfoffsets = m_VFList.getPatchOffsets();
    auto &vfdata = m_VFList.getVFData();
    auto &vfkoeff = m_VFList.getVFKoeff();

    PatchRef patch(m_Patches, i);
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

            // Take light from p to i
            patchSum[i] += getPatchBounce(patch2, bounce - 1) * (vf * vfkoeff[i]);

            // Take light from i to patch2
            patchSum[patch2] += getPatchBounce(i, bounce - 1) * (vf * vfkoeff[patch2]);

            AFW_ASSERT(patchSum[i].r >= 0 && patchSum[i].g >= 0 && patchSum[i].b >= 0);
            AFW_ASSERT(patchSum[patch2].r >= 0 && patchSum[patch2].g >= 0 && patchSum[patch2].b >= 0);

            dataOffset++;
        }

        poff += item.size;
    }
}

void rad::RadSim::sampleLightmap(size_t faceIdx) {
    Face &face = m_Faces[faceIdx];
    PatchTree &tree = m_PatchTrees[faceIdx];
    LightmapTexture &lm = m_Lightmaps[face.iLightmapIdx];
    AFW_ASSERT(face.hasLightmap());

    glm::vec2 baseGridSizeFloat = face.planeMaxBounds / m_flLuxelSize;

    for (int y = 0; y < face.vLightmapSize.y; y++) {
        for (int x = 0; x < face.vLightmapSize.x; x++) {
            glm::vec2 pos = glm::vec2(x, y) / baseGridSizeFloat * face.planeMaxBounds;

            float radius = std::max(m_flPatchSize, m_flLuxelSize) * 1.1f;
            glm::vec3 output = glm::vec3(0, 0, 0);

            if (tree.sampleLight(pos, radius, output)) {
                lm.getPixel({x, y}) = output;
            } else {
                // TODO: Fill the void with something
                lm.getPixel({x, y}) = glm::vec3(0, 0, 0);
                lm.getPixel({x, y}) = glm::vec3(1, 0, 1);
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
    return fmt::format("{}.svismat{}", m_LevelPath, m_PatchSizeStr);
}

std::string rad::RadSim::getVFListPath() {
    return fmt::format("{}.vflist{}", m_LevelPath, m_PatchSizeStr);
}
