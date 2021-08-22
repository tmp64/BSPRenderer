#include <algorithm>
#include <fstream>
#include <appfw/binary_file.h>
#include <appfw/timer.h>
#include <app_base/yaml.h>
#include <rad/rad_sim.h>
#include <rad/lightmap_writer.h>

//#define SAMPLE_NEIGHBOURS

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

}

rad::RadSim::RadSim()
    : m_VisMat(this)
    , m_VFList(this)
    , m_SVisMat(this) {
    printi("Using {} thread(s).", m_Executor.num_workers());
}

void rad::RadSim::setAppConfig(AppConfig *appcfg) {
    m_pAppConfig = appcfg;
    m_Config.loadFromAppConfig(*appcfg);
}

void rad::RadSim::setLevel(const bsp::Level *pLevel, const std::string &name,
                           const std::string &profileName) {
    m_pLevel = pLevel;
    m_LevelName = name;
    m_PatchHash = appfw::SHA256::Digest();
    appfw::SHA256 hash;

    // Create build directory
    fs::create_directories(getFileSystem().getFilePath(getBuildDirPath()));

    loadLevelConfig();
    loadBuildProfile(profileName);
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
    m_Bouncer.setup(this, m_Profile.iBounceCount);

    printi("Bouncing light...");
    appfw::Timer timer;
    timer.start();
    
    m_Bouncer.bounceLight();

    timer.stop();
    printi("Bounce light: {:.3} s", timer.dseconds());
}

void rad::RadSim::writeLightmaps() {
    LightmapWriter lmwriter;
    lmwriter.saveLightmap(this);
}

void rad::RadSim::loadLevelConfig() {
    m_LevelConfig.loadDefaults(m_Config);

    fs::path cfgPath = getFileSystem().findExistingFile(getLevelConfigPath(), std::nothrow);

    if (!cfgPath.empty()) {
        m_LevelConfig.loadLevelConfig(cfgPath);
    }
}

void rad::RadSim::loadBuildProfile(const std::string &profileName) {
    m_Profile = BuildProfile();

    std::ifstream radProfilesFile(getFileSystem().findExistingFile("assets:rad_profiles.yaml"));
    YAML::Node radProfiles = YAML::Load(radProfilesFile);

    // rad_profiles._common
    if (radProfiles["_common"]) {
        m_Profile.loadProfile(radProfiles["_common"]);
    }

    bool hasLevelProfiles = m_LevelConfig.yaml && m_LevelConfig.yaml["profiles"];
    bool profileWasFound = false;

    // profiles._common
    if (hasLevelProfiles && m_LevelConfig.yaml["profiles"]["_common"]) {
        m_Profile.loadProfile(m_LevelConfig.yaml["profiles"]["_common"]);
    }

    // rad_profiles.profile_name
    if (radProfiles[profileName]) {
        m_Profile.loadProfile(radProfiles[profileName]);
        profileWasFound = true;
    }

    // profiles.profile_name
    if (hasLevelProfiles && m_LevelConfig.yaml["profiles"][profileName]) {
        m_Profile.loadProfile(m_LevelConfig.yaml["profiles"][profileName]);
        profileWasFound = false;
    }

    if (!profileWasFound) {
        throw std::runtime_error(fmt::format("Profile '{}' not found", profileName));
    }

    m_Profile.finalize();
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

        // Add to plane
        m_Planes[face.iPlane].faces.push_back((unsigned)i);

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

            face.vPlaneOriginInPlaneCoords = {minX, minY};
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

        face.flPatchSize = (float)m_Profile.iBasePatchSize;

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

    printi("{} faces have lightmaps", lightmapCount);
}

void rad::RadSim::createPatches(appfw::SHA256 &hash) {
    printn("Creating patches...");
    appfw::Timer timer;
    timer.start();
    m_PatchTrees.resize(m_Faces.size());

    std::atomic<PatchIndex> totalPatchCount;

    for (size_t i = 0; i < m_Faces.size(); i++) {
        Face &face = m_Faces[i];

        if (!face.hasLightmap()) {
            continue;
        }

        // This can be running in multiple threads
        // (but it takes less than a second so it's fine)
        PatchTree &tree = m_PatchTrees[i];
        tree.createPatches(this, face, totalPatchCount, getMinPatchSize());
    }

    //-------------------------------------------------------------------------
    PatchIndex patchCount = totalPatchCount.load();
    m_Patches.allocate(patchCount);
    printi("Patch count: {}", patchCount);
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
}

void rad::RadSim::loadLevelEntities() {
    bool wasEnvLightSet = m_LevelConfig.sunLight.bIsSet;
    bool isEnvLightFound = false;

    for (auto &ent : m_pLevel->getEntities()) {
        std::string classname = ent.getValue<std::string>("classname", "");

        if (classname.empty()) {
            continue;
        }

        if (classname == "light") {

        } else if (classname == "light_environment") {
            if (!wasEnvLightSet) {
                if (!m_LevelConfig.sunLight.bIsSet) {
                    m_LevelConfig.sunLight.bIsSet = true;

                    // Read color
                    int color[4];
                    if (sscanf(ent.getValue<std::string>("_light").c_str(), "%d %d %d %d", &color[0], &color[1],
                               &color[2], &color[3]) != 4) {
                        throw std::runtime_error(fmt::format("light_environment _light = {} is invalid",
                                                             ent.getValue<std::string>("_light")));
                    }

                    m_LevelConfig.sunLight.vColor.r = color[0] / 255.f;
                    m_LevelConfig.sunLight.vColor.g = color[1] / 255.f;
                    m_LevelConfig.sunLight.vColor.b = color[2] / 255.f;

                    // Light color is interpreted as linear value in qrad
                    m_LevelConfig.sunLight.vColor = linearToGamma(m_LevelConfig.sunLight.vColor);
                    
                    m_LevelConfig.sunLight.flBrightness = color[3] / m_Config.flEnvLightDiv;

                    // Read angle
                    m_LevelConfig.sunLight.flYaw = ent.getValue<float>("angle");
                    m_LevelConfig.sunLight.flPitch = ent.getValue<float>("pitch");
                }

                if (isEnvLightFound) {
                    printw("Level has multiple light_environment. Only first one is used.");
                }

                isEnvLightFound = true;
            }
        }
    }
}

void rad::RadSim::updateProgress(double progress) {
    if (m_fnProgressCallback) {
        m_fnProgressCallback(progress);
    }
}

std::string rad::RadSim::getBuildDirPath() {
    return fmt::format("assets:mapsrc/build/{}", m_LevelName);
}

std::string rad::RadSim::getLevelConfigPath() {
    return fmt::format("assets:mapsrc/{}.rad.yaml", m_LevelName);
}

std::string rad::RadSim::getVisMatPath() {
    return fmt::format("{}/svismat{}.dat", getBuildDirPath(), m_Profile.iBasePatchSize);
}

std::string rad::RadSim::getVFListPath() {
    return fmt::format("{}/vflist{}.dat", getBuildDirPath(), m_Profile.iBasePatchSize);
}

std::string rad::RadSim::getLightmapPath() {
    return fmt::format("assets:maps/{}.bsp.lm", m_LevelName);
}

float rad::RadSim::gammaToLinear(float val) {
    return std::pow(val, m_LevelConfig.flGamma);
}

glm::vec3 rad::RadSim::gammaToLinear(const glm::vec3 &val) {
    return glm::pow(val, glm::vec3(m_LevelConfig.flGamma));
}

float rad::RadSim::linearToGamma(float val) {
    return std::pow(val, 1.0f / m_LevelConfig.flGamma);
}

glm::vec3 rad::RadSim::linearToGamma(const glm::vec3 &val) {
    return glm::pow(val, glm::vec3(1.0f / m_LevelConfig.flGamma));
}
