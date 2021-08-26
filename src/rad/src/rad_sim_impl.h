#ifndef RAD_SIM_IMPL_H
#define RAD_SIM_IMPL_H
#include <atomic>
#include <functional>
#include <memory>
#include <taskflow/taskflow.hpp>
#include <appfw/sha256.h>
#include <appfw/utils.h>
#include <bsp/level.h>
#include <rad/rad_sim.h>

#include "types.h"
#include "patch_list.h"
#include "patch_tree.h"
#include "vismat.h"
#include "sparse_vismat.h"
#include "vflist.h"
#include "bouncer.h"


namespace rad {

/**
 * Internal state of the radiosity simulator.
 */
class RadSimImpl : appfw::NoMove {
public:
    // Rad data
    AppConfig *m_pAppConfig = nullptr;
    RadSim::ProgressCallback m_fnProgressCallback;
    RadConfig m_Config;

    // Level data
    const bsp::Level *m_pLevel = nullptr;
    std::string m_LevelName;
    LevelConfig m_LevelConfig;
    BuildProfile m_Profile;
    std::vector<Plane> m_Planes;
    std::vector<Face> m_Faces;
    std::vector<PatchTree> m_PatchTrees;
    PatchList m_Patches;
    VisMat m_VisMat;
    SparseVisMat m_SVisMat;
    VFList m_VFList;
    Bouncer m_Bouncer;

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

    std::string getBuildDirPath();
    std::string getLevelConfigPath();
    std::string getVisMatPath();
    std::string getVFListPath();
    std::string getLightmapPath();

    float gammaToLinear(float val);
    glm::vec3 gammaToLinear(const glm::vec3 &val);
    float linearToGamma(float val);
    glm::vec3 linearToGamma(const glm::vec3 &val);

    static inline tf::Executor m_Executor;

private:

    //! Returns minimum allowed patch size
    inline float getMinPatchSize() { return m_Profile.flMinPatchSize; }

    void loadLevelConfig();
    void loadBuildProfile(const std::string &profileName);

    //! Loads planes from the BSP.
    void loadPlanes();

    //! Loads faces from the BSP.
    void loadFaces();

    //! Creates patches for every face.
    void createPatches(appfw::SHA256 &hash);

    //! Loads lights from entities.
    void loadLevelEntities();
};

} // namespace rad

#endif
