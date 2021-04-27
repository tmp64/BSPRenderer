#ifndef RAD_RAD_SIM_H
#define RAD_RAD_SIM_H
#include <functional>
#include <nlohmann/json.hpp>
#include <appfw/sha256.h>
#include <appfw/utils.h>
#include <appfw/thread_pool.h>
#include <bsp/level.h>
#include <app_base/app_config.h>
#include <rad/types.h>
#include <rad/patch_list.h>
#include <rad/vismat.h>
#include <rad/vflist.h>

namespace rad {

/**
 * Radiosity simulator.
 * Any method can throw an exception on error.
 * Not thread-safe. Only call from one thread at a time.
 */
class RadSim : appfw::NoMove {
public:
    using ProgressCallback = std::function<void(double progress)>;

    /**
     * Maximum distance from any point in world to a point on a sky face.
     */
    static constexpr float SKY_RAY_LENGTH = 8192.f;

    RadSim();

    /**
     * Sets app config used by the rad.
     */
    inline void setAppConfig(AppConfig *appcfg) { m_pAppConfig = appcfg; }

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
     * Sets the level.
     */
    void setLevel(const bsp::Level *pLevel, const std::string &path);

    /**
     * Returns size of a patch in units.
     */
    inline float getPatchSize() { return m_flPatchSize; }

    /**
     * Returns hash of the level with this particular configuration of patches (position, count, etc).
     * Valid after loadLevelConfig.
     */
    inline appfw::SHA256::Digest getPatchHash() { return m_PatchHash; }

    /**
     * Loads config file for the level.
     * Creates patches for faces.
     * Can take some time.
     */
    void loadLevelConfig();

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
     * Gets bounce count.
     */
    inline int getBounceCount() { return m_iBounceCount; }

    /**
     * Sets bounce count. Higher value means better global illumination.
     */
    inline void setBounceCount(int count) { m_iBounceCount = count; }

    /**
     * Adds lighting and bounces light around patches.
     */
    void bounceLight();

    /**
     * Exports final lightmaps into a file.
     */
    void writeLightmaps();

private:
    appfw::ThreadPool m_ThreadPool;
    appfw::ThreadDispatcher m_Dispatcher;
    AppConfig *m_pAppConfig = nullptr;
    ProgressCallback m_fnProgressCallback;

    const bsp::Level *m_pLevel = nullptr;
    std::string m_LevelPath;
    std::vector<Plane> m_Planes;
    std::vector<Face> m_Faces;
    std::vector<LightmapTexture> m_Lightmaps;
    PatchList m_Patches;
    VisMat m_VisMat;
    VFList m_VFList;
    std::vector<glm::vec3> m_PatchBounce;

    nlohmann::json m_LevelConfigJson;
    float m_flPatchSize = 0;
    float m_flReflectivity = 0;
    float m_flGamma = 0;
    std::string m_PatchSizeStr;
    EnvLight m_EnvLight;

    int m_iBounceCount = 0;

    appfw::SHA256::Digest m_PatchHash = {};

    /**
     * Loads planes from the BSP.
     */
    void loadPlanes();

    /**
     * Loads faces from the BSP.
     */
    void loadFaces();

    /**
     * Creates patches for every face.
     */
    void createPatches(appfw::SHA256 &hash);

    /**
     * Loads lights from entities.
     */
    void loadLevelEntities();

    /**
     * Converts color from gamma space to linear space.
     */
    glm::vec3 correctColorGamma(const glm::vec3 &color);

    /**
     * Returns reference to color of patch in specified bounce.
     * Bounce 0 is initial color.
     */
    glm::vec3 &getPatchBounce(size_t patch, size_t bounce);

    /**
     * Adds initial lighting to patches (texlights, entity lights).
     */
    void addLighting();

    /**
     * Clears array for light bouncing.
     */
    void clearBounceArray();

    /**
     * Applies environment lighting to patches.
     */
    void applyEnvLight();

    /**
     * Applies texture lighting to patches.
     */
    void applyTexLights();

    /**
     * Calls the progress callback with specified progress.
     */
    void updateProgress(double progress);

    std::string getVisMatPath();
    std::string getVFListPath();

    friend class VisMat;
    friend class VFList;
};

} // namespace rad

#endif
