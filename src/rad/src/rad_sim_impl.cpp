#include "rad_sim_impl.h"
#include <algorithm>
#include <fstream>
#include <app_base/yaml.h>
#include <appfw/binary_file.h>
#include <appfw/timer.h>
#include <appfw/str_utils.h>
#include <bsp/utils.h>
#include "lightmap_writer.h"
#include "patch_divider.h"

rad::RadSimImpl::RadSimImpl()
    : m_VisMat(*this)
    , m_VFList(*this)
    , m_SVisMat(*this)
    , m_Bouncer(*this) {
    printi("Using {} thread(s).", m_Executor.num_workers());
}

void rad::RadSimImpl::setAppConfig(AppConfig *appcfg) {
    m_pAppConfig = appcfg;
    m_Config.loadFromAppConfig(*appcfg);
}

void rad::RadSimImpl::setLevel(const bsp::Level *pLevel, const std::string &name,
                               const std::string &profileName) {
    m_pLevel = pLevel;
    m_LevelName = name;
    m_PatchHash = appfw::SHA256::Digest();
    appfw::SHA256 hash;

    m_Entities.loadFromString(pLevel->getEntitiesLump());

    if (m_Entities.empty()) {
        throw std::runtime_error("Map has no entities");
    }

    // Create build directory
    fs::create_directories(getFileSystem().getFilePath(getBuildDirPath()));

    loadLevelConfig();
    loadBuildProfile(profileName);
    loadWADs();
    loadMaterials();
    loadPlanes();
    loadFaces();
    createPatches(hash);
    loadLevelEntities();
    samplePatchReflectivity();

    m_PatchHash = hash.digest();
}

bool rad::RadSimImpl::isVisMatValid() {
    return m_SVisMat.isValid();
}

bool rad::RadSimImpl::loadVisMat() {
    fs::path path = getFileSystem().findExistingFile(getVisMatPath(), std::nothrow);

    if (!path.empty()) {
        m_SVisMat.loadFromFile(path);
        return m_SVisMat.isValid();
    }

    return false;
}

void rad::RadSimImpl::calcVisMat() {
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

bool rad::RadSimImpl::isVFListValid() {
    return m_VFList.isValid();
}

void rad::RadSimImpl::calcViewFactors() {
    if (!isVisMatValid()) {
        throw std::logic_error("valid vismat required");
    }

    m_VFList.calculateVFList();
}

void rad::RadSimImpl::bounceLight() {
    printi("Applying lighting...");
    m_Bouncer.setup(m_Profile.iBounceCount);

    printi("Bouncing light...");
    appfw::Timer timer;
    timer.start();

    m_Bouncer.bounceLight();

    timer.stop();
    printi("Bounce light: {:.3} s", timer.dseconds());
}

void rad::RadSimImpl::writeLightmaps() {
    LightmapWriter lmwriter(*this);
    lmwriter.saveLightmap();
}

void rad::RadSimImpl::loadLevelConfig() {
    m_LevelConfig.loadDefaults(m_Config);

    fs::path cfgPath = getFileSystem().findExistingFile(getLevelConfigPath(), std::nothrow);
    if (!cfgPath.empty()) {
        m_LevelConfig.loadLevelConfig(cfgPath);
    }

    fs::path surfPath = getFileSystem().findExistingFile(getSurfaceConfigPath(), std::nothrow);
    if (!surfPath.empty()) {
        std::ifstream file(surfPath);
        m_SurfaceConfig = YAML::Load(file);
    }
}

void rad::RadSimImpl::loadBuildProfile(const std::string &profileName) {
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

void rad::RadSimImpl::loadWADs() {
    auto &ws = m_Entities[0];
    std::string wads = ws.get("wad").asString();
    std::vector<std::string> wadList = bsp::parseWadListString(wads);

    for (const std::string &wadName : wadList) {
        fs::path path = getFileSystem().findExistingFile("assets:" + wadName, std::nothrow);

        if (path.empty()) {
            printe("WAD file {} not found", wadName);
            continue;
        }

        bsp::WADFile &wadFile = m_Wads[wadName];
        wadFile.loadFromFile(path);
    }

    printi("Loaded {} WAD files", m_Wads.size());
}

void rad::RadSimImpl::loadMaterials() {
    m_MatPropLoader.init(m_LevelName, "assets:sound/materials.txt", "assets:materials");
    auto &textures = m_pLevel->getTextures();
    m_Materials.resize(textures.size());

    for (size_t i = 0; i < textures.size(); i++) {
        Material &material = m_Materials[i];
        const bsp::BSPMipTex &tex = textures[i];
        const bsp::WADTexture *pWadTex = nullptr;
        std::string wadName;

        // Find the texture in WADs
        for (auto &[name, wad] : m_Wads) {
            auto &wadTextures = wad.getTextures();

            for (const bsp::WADTexture &wadTex : wadTextures) {
                if (!appfw::strcasecmp(wadTex.getName(), tex.szName)) {
                    // Found the texture
                    pWadTex = &wadTex;
                    wadName = name;
                    break;
                }
            }

            if (pWadTex) {
                break;
            }
        }

        std::string texName = tex.szName;
        appfw::strToLower(texName.begin(), texName.end());

        if (pWadTex) {
            // Texture is in the WAD
            material.loadProps(m_MatPropLoader, texName, wadName);
            material.loadFromWad(*this, *pWadTex);
        } else {
            // Texture is in the level
            material.loadProps(m_MatPropLoader, texName, m_LevelName + ".bsp");
        }
    }

    printi("Loaded {} materials", m_Materials.size());
}

void rad::RadSimImpl::loadPlanes() {
    m_Planes.resize(m_pLevel->getPlanes().size());

    for (size_t i = 0; i < m_pLevel->getPlanes().size(); i++) {
        m_Planes[i].load(*this, m_pLevel->getPlanes()[i]);
    }
}

void rad::RadSimImpl::loadFaces() {
    m_Faces.resize(m_pLevel->getFaces().size());

    size_t lightmapCount = 0;

    for (size_t i = 0; i < m_pLevel->getFaces().size(); i++) {
        m_Faces[i].load(*this, m_pLevel->getFaces()[i], (int)i);

        // Add to plane
        m_Planes[m_Faces[i].iPlane].faces.push_back((unsigned)i);

        if (m_Faces[i].hasLightmap()) {
            lightmapCount++;
        }
    }

    printi("{} faces have lightmaps", lightmapCount);
}

void rad::RadSimImpl::createPatches(appfw::SHA256 &hash) {
    printn("Creating patches...");
    appfw::Timer timer;
    timer.start();

    PatchDivider divider;
    uint64_t totalPatchCount = 0;

    for (size_t i = 0; i < m_Faces.size(); i++) {
        Face &face = m_Faces[i];

        if (!face.hasLightmap()) {
            continue;
        }

        totalPatchCount += divider.createPatches(this, face, (PatchIndex)totalPatchCount);

        if (totalPatchCount > MAX_PATCH_COUNT) {
            throw std::runtime_error(
                fmt::format("Error: Patch limit reached ({})", MAX_PATCH_COUNT));
        }
    }

    //-------------------------------------------------------------------------
    PatchIndex patchCount = (PatchIndex)totalPatchCount;
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
    [[maybe_unused]] PatchIndex transferPatchesCount = 0;
    transferPatchesCount = divider.transferPatches(this, hash);
    AFW_ASSERT_REL(transferPatchesCount == m_Patches.size());

    timer.stop();
    printi("Create patches: {:.3} s", timer.dseconds());
}

void rad::RadSimImpl::loadLevelEntities() {
    bool wasEnvLightSet = m_LevelConfig.sunLight.bIsSet;
    bool isEnvLightFound = false;

    for (auto &ent : m_Entities) {
        std::string_view classname = ent.getClassName();

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
                    const std::string &lightValue = ent.get("_light").asString();
                    if (sscanf(lightValue.c_str(), "%d %d %d %d", &color[0], &color[1], &color[2],
                               &color[3]) != 4) {
                        throw std::runtime_error(
                            fmt::format("light_environment _light = {} is invalid", lightValue));
                    }

                    m_LevelConfig.sunLight.vColor.r = color[0] / 255.f;
                    m_LevelConfig.sunLight.vColor.g = color[1] / 255.f;
                    m_LevelConfig.sunLight.vColor.b = color[2] / 255.f;

                    // Light color is interpreted as linear value in qrad
                    m_LevelConfig.sunLight.vColor = linearToGamma(m_LevelConfig.sunLight.vColor);

                    m_LevelConfig.sunLight.flBrightness = color[3] / m_Config.flEnvLightDiv;

                    // Read angle
                    int anglesIdx = ent.indexOf("angles");
                    if (anglesIdx != -1) {
                        std::string angles = ent.get(anglesIdx).asString();
                        int pitch, yaw, roll;
                        if (sscanf(angles.c_str(), "%d %d %d", &pitch, &yaw, &roll) != 3) {
                            throw std::runtime_error(
                                fmt::format("light_environment angles = '{}' is invalid", angles));
                        }

                        m_LevelConfig.sunLight.flYaw = (float)yaw;
                        m_LevelConfig.sunLight.flPitch = (float)pitch;
                    } else {
                        m_LevelConfig.sunLight.flYaw = ent.get("angle").asFloat();
                        m_LevelConfig.sunLight.flPitch = ent.get("pitch").asFloat();
                    }
                }

                if (isEnvLightFound) {
                    printw("Level has multiple light_environment. Only first one is used.");
                }

                isEnvLightFound = true;
            }
        }
    }
}

void rad::RadSimImpl::samplePatchReflectivity() {
    auto fnSampleFacePatches = [&](size_t faceIndex) {
        Face &face = m_Faces[faceIndex];
        const bsp::BSPTextureInfo &texInfo = m_pLevel->getTexInfo()[face.iTextureInfo];
        Material &material = *face.pMaterial;
        PatchIndex lastPatch = face.iFirstPatch + face.iNumPatches;

        for (PatchIndex i = face.iFirstPatch; i < lastPatch; i++) {
            PatchRef patch(m_Patches, i);
            glm::vec2 texPos, texScale;
            float patchSize = patch.getSize();
            texPos.s = glm::dot(patch.getOrigin(), texInfo.vS) + texInfo.fSShift;
            texPos.t = glm::dot(patch.getOrigin(), texInfo.vT) + texInfo.fTShift;
            texScale.s = std::max(patchSize * glm::length(texInfo.vS), 1.0f);
            texScale.t = std::max(patchSize * glm::length(texInfo.vT), 1.0f);

            patch.getReflectivity() *= material.sampleColor(texPos, texScale);
        }
    };

    printi("Sampling textures for patch reflectivity...");
#if 1
    tf::Taskflow taskflow;
    taskflow.for_each_index_dynamic((size_t)0, m_Faces.size(), (size_t)1, fnSampleFacePatches);
    m_Executor.run(taskflow).wait();
#else
    // Run in signle thread for easier debugging
    for (size_t i = 0; i < m_Faces.size(); i++) {
        fnSampleFacePatches(i);
    }
#endif
}

void rad::RadSimImpl::updateProgress(double progress) {
    if (m_fnProgressCallback) {
        m_fnProgressCallback(progress);
    }
}

int rad::RadSimImpl::traceLine(glm::vec3 from, glm::vec3 to) {
    return m_pLevel->traceLine(from, to);
}

std::string rad::RadSimImpl::getBuildDirPath() {
    return fmt::format("assets:mapsrc/build/{}", m_LevelName);
}

std::string rad::RadSimImpl::getLevelConfigPath() {
    return fmt::format("assets:mapsrc/{}.rad.yaml", m_LevelName);
}

std::string rad::RadSimImpl::getSurfaceConfigPath() {
    return fmt::format("assets:mapsrc/{}.rad.surf.yaml", m_LevelName);
}

std::string rad::RadSimImpl::getVisMatPath() {
    return fmt::format("{}/svismat{}.dat", getBuildDirPath(), m_Profile.iBasePatchSize);
}

std::string rad::RadSimImpl::getVFListPath() {
    return fmt::format("{}/vflist{}.dat", getBuildDirPath(), m_Profile.iBasePatchSize);
}

std::string rad::RadSimImpl::getLightmapPath() {
    return fmt::format("assets:maps/{}.bsp.lm", m_LevelName);
}

float rad::RadSimImpl::gammaToLinear(float val) {
    return std::pow(val, m_LevelConfig.flGamma);
}

glm::vec3 rad::RadSimImpl::gammaToLinear(const glm::vec3 &val) {
    return glm::pow(val, glm::vec3(m_LevelConfig.flGamma));
}

float rad::RadSimImpl::linearToGamma(float val) {
    return std::pow(val, 1.0f / m_LevelConfig.flGamma);
}

glm::vec3 rad::RadSimImpl::linearToGamma(const glm::vec3 &val) {
    return glm::pow(val, glm::vec3(1.0f / m_LevelConfig.flGamma));
}
