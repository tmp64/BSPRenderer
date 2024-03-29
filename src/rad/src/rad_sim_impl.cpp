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
#include "bouncer.h"

rad::RadSimImpl::RadSimImpl(int threadCount)
    : m_VisMat(*this)
    , m_VFList(*this)
    , m_SVisMat(*this) {
    if (threadCount == -1) {
        threadCount = std::thread::hardware_concurrency();

        if (threadCount == 0) {
            threadCount = 1;
            printw("Failed to detect thread count, assuming {}", threadCount);
        }
    }

    m_pExecutor = std::make_unique<tf::Executor>(threadCount);
    printi("Using {} thread(s).", m_pExecutor->num_workers());
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
    preloadBrushModels();
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
    printn("Bouncing light...");
    Bouncer bouncer(*this);

    for (int i = 0; i <= m_iMaxLightstyle; i++) {
        LightStyle &ls = m_LightStyles[i];
        if (!ls.hasLights()) {
            continue;
        }

        appfw::Timer timer;

        int bounceCount = ls.iBounceCount == -1 ? m_Profile.iBounceCount : ls.iBounceCount;
        bounceCount = std::min(bounceCount, m_Profile.iBounceCount);

        printi("{}: {} entlights, {} texlights, {} bounces", i, ls.entlights.size(), ls.texlights.size(), bounceCount);

        bouncer.setup(i, bounceCount);

        if (i == 0) {
            bouncer.addSunLight();
            bouncer.addSkyLight();
        }

        for (int faceIdx : ls.texlights) {
            bouncer.addTexLight(faceIdx);
        }

        for (EntLight &el : ls.entlights) {
            bouncer.addEntLight(el);
        }

        bouncer.calcLight();

        timer.stop();
        printi("    ... {:.3f} s", timer.dseconds());
    }
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

void rad::RadSimImpl::preloadBrushModels() {
    m_FaceTranslations.resize(m_pLevel->getFaces().size());

    for (auto &ent : m_Entities) {
        try {
            // See if has brush model
            int kvModel = ent.indexOf("model");
            if (kvModel != -1) {
                const std::string &model = ent.get(kvModel).asString();

                if (model.length() >= 2 && model[0] == '*') {
                    parseBrushModel(ent, std::stoi(model.substr(1)));
                }
            }
        } catch (const std::exception &e) {
            printe("Failed to parse model in an entity: {}", e.what());
        }
    }

    auto &models = m_pLevel->getModels();
    if (models.size() > 1) {
        m_iFirstBModelFace = models[1].iFirstFace;
        m_iLastBModelFace = (int)m_FaceTranslations.size();
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
    std::vector<glm::vec3>().swap(m_FaceTranslations);
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
    // Set initial bounce count for all lightstyles
    m_LightStyles[0].iBounceCount = m_Profile.iBounceCount;
    for (int i = 1; i < MAX_LIGHTSTYLES; i++) {
        m_LightStyles[i].iBounceCount = m_Config.iLightBounce;
    }

    // Add texlights to lightstyles
    // TODO: Allow to change lightstyle idx
    for (size_t i = 0; i < m_Faces.size(); i++) {
        int lightstyle = 0;

        if (!isNullVector(m_Faces[i].vLightColor)) {
            m_LightStyles[lightstyle].texlights.push_back(int(i));
        }
    }

    // Parse all entities
    for (auto &ent : m_Entities) {
        std::string classname = ent.getClassName();

        if (classname.empty()) {
            continue;
        }

        std::string_view cn = classname;

        try {
            if (cn == "light_environment") {
                addEnvLightEntity(ent);
            } else if (cn.substr(0, 5) == "light") {
                addLightEntity(ent);
            }
        } catch (const std::exception &e) {
            printe("Entity failed to parse: {}", e.what());
        }
    }

    // Apply env lighting from the config
    if (m_LevelConfig.sunLight.bIsSet) {
        const auto &sun = m_LevelConfig.sunLight;
        m_SunLight.vLight = gammaToLinear(sun.vColor) * sun.flBrightness;
        m_SunLight.vDirection = getDirectionFromAngles(sun.flPitch, sun.flYaw);
    }

    float skyBrightness = m_LevelConfig.skyLight.flBrightnessMul != -1
                              ? m_LevelConfig.skyLight.flBrightnessMul
                              : m_Config.flSkyLightBrightness;

    if (m_LevelConfig.skyLight.vColor.r != -1) {
        m_SkyLight.vLight = m_LevelConfig.skyLight.vColor * skyBrightness;
    }
}

void rad::RadSimImpl::addLightEntity(bsp::EntityKeyValues &kv) {
    std::string classname = kv.getClassName();
    std::string targetname = kv.getTargetName();

    // Read color
    glm::vec4 light = kv.get("_light").asFloat4();
    glm::vec3 color = entLightColorToGamma(light / 255.0f);
    float intensity = light.a * m_Config.flEntLightScale;

    // Create ent light
    EntLight el;
    el.vOrigin = kv.get("origin").asFloat3();
    el.vLight = gammaToLinear(color) * intensity;

    if (classname == "light") {
        el.type = LightType::Point;
    } else {
        throw std::runtime_error("Invalid light type: " + classname);
    }

    // Read entity properties
    int lightstyle = 0;
    int kvStyle = kv.indexOf("style");

    if (kvStyle != -1) {
        lightstyle = kv.get(kvStyle).asInt();
    }

    m_iMaxLightstyle = std::max(lightstyle, m_iMaxLightstyle);

    m_LightStyles[lightstyle].entlights.push_back(std::move(el));
}

void rad::RadSimImpl::addEnvLightEntity(bsp::EntityKeyValues &kv) {
    // Read color
    glm::vec4 light = kv.get("_light").asFloat4();
    glm::vec3 color = entLightColorToGamma(light / 255.0f);
    float intensity = light.a / m_Config.flEnvLightDiv;

    // Read angle
    float pitch = 0, yaw = 0;
    int kvAngles = kv.indexOf("angles");

    if (kvAngles != -1) {
        glm::ivec3 angles = kv.get(kvAngles).asInt3();
        yaw = (float)angles.y;
        pitch = (float)angles.x;
    } else {
        yaw = kv.get("angle").asFloat();
        pitch = kv.get("pitch").asFloat();
    }

    m_SunLight.vLight = gammaToLinear(color) * intensity;
    m_SunLight.vDirection = getDirectionFromAngles(pitch, yaw);

    m_SkyLight.vLight = m_SunLight.vLight * m_Config.flSkyLightBrightness;
}

void rad::RadSimImpl::parseBrushModel(bsp::EntityKeyValues &kv, int modelIdx) {
    int kvOrigin = kv.indexOf("origin");
    if (kvOrigin != -1) {
        // Translate all faces of the model
        glm::vec3 origin = kv.get(kvOrigin).asFloat3();
        const bsp::BSPModel &model = m_pLevel->getModels().at(modelIdx);
        
        for (int i = model.iFirstFace; i < model.iFirstFace + model.nFaces; i++) {
            m_FaceTranslations[i] = origin;
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

            // A bit of gamma "correction" to increase the reflectivity
            glm::vec3 texel = material.sampleColor(texPos, texScale);
            float intensity = glm::dot(texel, RGB_INTENSITY);

            if (intensity >= 0.0001f) {
                float adjustedIntensity = std::pow(intensity, 1.0f / TEXTURE_GAMMA_ADJ);
                texel *= adjustedIntensity / intensity;
            }

            patch.getReflectivity() *= texel;
        }
    };

    printi("Sampling textures for patch reflectivity...");
#if 1
    tf::Taskflow taskflow;
    taskflow.for_each_index_dynamic((size_t)0, m_Faces.size(), (size_t)1, fnSampleFacePatches);
    m_pExecutor->run(taskflow).wait();
#else
    // Run in signle thread for easier debugging
    for (size_t i = 0; i < m_Faces.size(); i++) {
        fnSampleFacePatches(i);
    }
#endif
}

#if 0
// Lightstyles are give out by csg compiler, not rad
int rad::RadSimImpl::findOrAllocateLightStyle(std::string targetName, std::string pattern,
                                              int origLightstyle) {
    if (targetName.empty() && pattern.empty() && origLightstyle < RESERVED_LIGHTSTYLES) {
        // Non-toggleable lighting
        return origLightstyle;
    }

    if (origLightstyle > 0 && origLightstyle < RESERVED_LIGHTSTYLES && pattern.empty()) {
        pattern = "__reserved" + std::to_string(origLightstyle);
    }

    // Find a lightstyle with targetName and pattern
    for (int i = 0; i < std::size(m_LightStyles); i++) {
        LightStyle &ls = m_LightStyles[i];

        if (ls.name == targetName && ls.pattern == pattern) {
            return i;
        }
    }

    // Allocate a new one
    if (m_iNumLightstyles == MAX_LIGHTSTYLES) {
        return -1;
    }

    return m_iNumLightstyles++;
}
#endif

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

glm::vec3 rad::RadSimImpl::entLightColorToGamma(const glm::vec3 &val) {
    return glm::pow(val, glm::vec3(1.0f / ENT_LIGHT_GAMMA));
}
