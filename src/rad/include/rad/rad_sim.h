#ifndef RAD_RAD_SIM_H
#define RAD_RAD_SIM_H
#include <atomic>
#include <functional>
#include <memory>
#include <taskflow/taskflow.hpp>
#include <appfw/sha256.h>
#include <appfw/utils.h>
#include <bsp/level.h>
#include <app_base/app_config.h>
#include <rad/types.h>
#include <rad/patch_list.h>
#include <rad/patch_tree.h>
#include <rad/vismat.h>
#include <rad/sparse_vismat.h>
#include <rad/vflist.h>
#include <rad/bouncer.h>
#include <rad/rad_config.h>
#include <rad/level_config.h>

namespace rad {

/**
 * Radiosity simulator.
 * Any method can throw an exception on error.
 * Not thread-safe. Only call from one thread at a time.
 */
class RadSim : appfw::NoMove {
public:
    using ProgressCallback = std::function<void(double progress)>;

    RadSim();

    /**
     * Sets app config used by the rad.
     */
    void setAppConfig(AppConfig *appcfg);

    /**
     * This callback will be called to update progress for long tasks.
     * Called from the same thread that called methods bellow.
     * On end called with value of 1 (maybe multiple times).
     */
    inline void setProgressCallback(const ProgressCallback &cb) { m_fnProgressCallback = cb; }

    /**
     * Returns level light is calculated for.
     */
    inline const bsp::Level *getLevel() { return m_pLevel; }

    /**
     * Sets and loads the level.
     */
    void setLevel(const bsp::Level *pLevel, const std::string &name, const std::string &profileName);

    /**
     * Returns the build profile.
     */
    inline const BuildProfile &getBuildProfile() { return m_Profile; }

    /**
     * Sets the bounce count.
     */
    inline void setBounceCount(int count) { m_Profile.iBounceCount = count; }

    /**
     * Returns hash of the level with this particular configuration of patches (position, count, etc).
     * Valid after loadLevelConfig.
     */
    inline appfw::SHA256::Digest getPatchHash() { return m_PatchHash; }

    /**
     * Returns whether a valid visibility matrix is loaded.
     * If false, call loadVisMat.
     */
    bool isVisMatValid();

    /**
     * Loads the vismat if it exists and valid or returns false (call calcVisMat in that case).
     * On loading error throws an exception.
     */
    bool loadVisMat();

    /**
     * Recalculates vismat and saves it into the file.
     * Takes a lot of time.
     */
    void calcVisMat();

    /**
     * Returns whether viewfactor list is valid.
     * If false, call loadVFList.
     */
    bool isVFListValid();

    /**
     * Loads the vflist if it exists and valid or returns false (call calcViewFactors in that case).
     * On loading error throws an exception.
     * Doesn't require vismat.
     */
    bool loadVFList();

    /**
     * Calculates viewfactors between patches.
     * Requires vismat.
     */
    void calcViewFactors();

    /**
     * Adds lighting and bounces light around patches.
     */
    void bounceLight();

    /**
     * Exports final lightmaps into a file.
     */
    void writeLightmaps();

private:
    // Rad data
    AppConfig *m_pAppConfig = nullptr;
    ProgressCallback m_fnProgressCallback;
    RadConfig m_Config;

    // Level data
    const bsp::Level *m_pLevel = nullptr;
    std::string m_LevelName;
    LevelConfig m_LevelConfig;
    BuildProfile m_Profile;
    std::vector<Plane> m_Planes;
    std::vector<Face> m_Faces;
    std::vector<PatchTree> m_PatchTrees;
    std::vector<LightmapTexture> m_Lightmaps;
    PatchList m_Patches;
    VisMat m_VisMat;
    SparseVisMat m_SVisMat;
    VFList m_VFList;
    Bouncer m_Bouncer;

    appfw::SHA256::Digest m_PatchHash = {};

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

    //! Writes lightmap data into the lightmap texture
    void sampleLightmap(size_t faceIdx);

    //! Calls the progress callback with specified progress.
    void updateProgress(double progress);

    std::string getBuildDirPath();
    std::string getLevelConfigPath();
    std::string getVisMatPath();
    std::string getVFListPath();
    std::string getLightmapPath();

    float gammaToLinear(float val);
    glm::vec3 gammaToLinear(const glm::vec3 &val);

    static inline tf::Executor m_Executor;

    friend class VisMat;
    friend class SparseVisMat;
    friend class VFList;
    friend class PatchTree;
    friend class Bouncer;
};

} // namespace rad

#endif
