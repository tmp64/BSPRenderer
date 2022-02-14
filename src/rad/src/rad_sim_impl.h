#ifndef RAD_SIM_IMPL_H
#define RAD_SIM_IMPL_H
#include <atomic>
#include <functional>
#include <memory>
#include <taskflow/taskflow.hpp>
#include <appfw/sha256.h>
#include <appfw/utils.h>
#include <bsp/level.h>
#include <bsp/wad_file.h>
#include <bsp/entity_key_values.h>
#include <material_props/material_prop_loader.h>
#include <rad/rad_sim.h>

#include "types.h"
#include "patch_list.h"
#include "vismat.h"
#include "sparse_vismat.h"
#include "vflist.h"
#include "material.h"

namespace rad {

//! Gamma used by the GoldSrc engine when displaying lightmaps.
//! qrad encodes lightmaps with gamma of 2 but engine displays them with lightgamma = 2.5.
//! qrad also interprets color values of entities in linear space.
constexpr float ENT_LIGHT_GAMMA = 2.5f;

//! Maximum number of lightstyles. Engine limit.
constexpr int MAX_LIGHTSTYLES = 255;

//! Number of reserved lightstyles
constexpr int RESERVED_LIGHTSTYLES = 32;

/**
 * Internal state of the radiosity simulator.
 */
class RadSimImpl : appfw::NoMove {
public:
    struct SunLight {
        glm::vec3 vLight = glm::vec3(0, 0, 0); //!< Linear color times intensity
        glm::vec3 vDirection = glm::vec3(0, 0, 0);
    };

    struct SkyLight {
        glm::vec3 vLight = glm::vec3(0, 0, 0); //!< Linear color times intensity
    };

    // Rad data
    AppConfig *m_pAppConfig = nullptr;
    RadSim::ProgressCallback m_fnProgressCallback;
    RadConfig m_Config;

    // Level data
    const bsp::Level *m_pLevel = nullptr;
    std::string m_LevelName;
    LevelConfig m_LevelConfig;
    YAML::Node m_SurfaceConfig;

    BuildProfile m_Profile;

    std::map<std::string, bsp::WADFile> m_Wads;
    MaterialPropLoader m_MatPropLoader;
    std::vector<Material> m_Materials;
    std::vector<Plane> m_Planes;
    std::vector<Face> m_Faces;

    bsp::EntityKeyValuesDict m_Entities;
    LightStyle m_LightStyles[MAX_LIGHTSTYLES];
    int m_iMaxLightstyle = 0;
    SunLight m_SunLight;
    SkyLight m_SkyLight;

    PatchList m_Patches;
    VisMat m_VisMat;
    SparseVisMat m_SVisMat;
    VFList m_VFList;

    appfw::SHA256::Digest m_PatchHash = {};

    RadSimImpl();

    //! Sets app config used by the rad.
    void setAppConfig(AppConfig *appcfg);

    //! This callback will be called to update progress for long tasks.
    //! Called from the same thread that called methods bellow.
    //! On end called with value of 1 (maybe multiple times).
    inline void setProgressCallback(const RadSim::ProgressCallback &cb) { m_fnProgressCallback = cb; }

    //! Returns level light is calculated for.
    inline const bsp::Level *getLevel() { return m_pLevel; }

    //! Sets and loads the level.
    void setLevel(const bsp::Level *pLevel, const std::string &name,
                  const std::string &profileName);

    //! Returns the build profile.
    inline const BuildProfile &getBuildProfile() { return m_Profile; }

    //! Sets the bounce count.
    inline void setBounceCount(int count) { m_Profile.iBounceCount = count; }

    //! Returns hash of the level with this particular configuration of patches (position, count,
    //! etc). Valid after loadLevelConfig.
    inline appfw::SHA256::Digest getPatchHash() { return m_PatchHash; }

    //! Returns minimum allowed patch size
    inline float getMinPatchSize() { return m_Profile.flMinPatchSize; }

    //! Returns whether a valid visibility matrix is loaded.
    //! If false, call loadVisMat.
    bool isVisMatValid();

    //! Loads the vismat if it exists and valid or returns false (call calcVisMat in that case).
    //! On loading error throws an exception.
    bool loadVisMat();

    //! Recalculates vismat and saves it into the file.
    //! Takes a lot of time.
    void calcVisMat();

    //! Returns whether viewfactor list is valid.
    //! If false, call loadVFList.
    bool isVFListValid();

    //! Calculates viewfactors between patches.
    //! Requires vismat.
    void calcViewFactors();

    //! Adds lighting and bounces light around patches.
    void bounceLight();

    //! Exports final lightmaps into a file.
    void writeLightmaps();

    //! Calls the progress callback with specified progress.
    void updateProgress(double progress);

    //! Checks if there is a clear path between two points.
    //! @returns (CONTENTS_SOLID or CONTENTS_SKY) or CONTENTS_EMPTY if didn't hit.
    int traceLine(glm::vec3 from, glm::vec3 to);

    std::string getBuildDirPath();
    std::string getLevelConfigPath();
    std::string getSurfaceConfigPath();
    std::string getVisMatPath();
    std::string getVFListPath();
    std::string getLightmapPath();

    float gammaToLinear(float val);
    glm::vec3 gammaToLinear(const glm::vec3 &val);
    float linearToGamma(float val);
    glm::vec3 linearToGamma(const glm::vec3 &val);

    //! Converts light entity color to gamma space.
    //! Entity color is linear but displayed with gamma of 2.5
    glm::vec3 entLightColorToGamma(const glm::vec3 &val);

    //! Executes func for each patch that is visible from patch i, starting with i + 1 to patchCount.
    //! void func(PatchRef j)
    template <typename F>
    void forEachVisiblePatch(PatchIndex i, F func) {
        AFW_ASSERT(isVisMatValid());
        PatchIndex itemCount = m_SVisMat.getCountTable()[i];
        size_t offset = m_SVisMat.getOffsetTable()[i];
        auto &listItems = m_SVisMat.getListItems();

        PatchRef patch(m_Patches, i);
        PatchIndex poff = i + 1;

        for (size_t j = 0; j < itemCount; j++) {
            const SparseVisMat::ListItem &item = listItems[offset + j];
            poff += item.offset;

            for (PatchIndex k = 0; k < item.size; k++) {
                func(PatchRef(m_Patches, poff + k));
            }

            poff += item.size;
        }
    }

    static inline tf::Executor m_Executor;

private:

    void loadLevelConfig();
    void loadBuildProfile(const std::string &profileName);
    void loadWADs();
    void loadMaterials();

    //! Loads planes from the BSP.
    void loadPlanes();

    //! Loads faces from the BSP.
    void loadFaces();

    //! Creates patches for every face.
    void createPatches(appfw::SHA256 &hash);

    //! Loads lights from entities.
    void loadLevelEntities();
    void addLightEntity(bsp::EntityKeyValues &kv);
    void addEnvLightEntity(bsp::EntityKeyValues &kv);

    void samplePatchReflectivity();

    //! Finds a lightstyle or allocates a new one.
    //! @returns lightstyle index or -1 if limit is reached.
    //int findOrAllocateLightStyle(std::string targetName, std::string pattern, int origLightstyle);
};

} // namespace rad

#endif
