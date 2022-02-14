#ifndef RAD_RAD_SIM_H
#define RAD_RAD_SIM_H
#include <memory>
#include <appfw/sha256.h>
#include <appfw/utils.h>
#include <bsp/level.h>
#include <rad/rad_config.h>
#include <rad/level_config.h>

namespace rad {

class RadSimImpl;

/**
 * Radiosity simulator.
 * Any method can throw an exception on error.
 * Not thread-safe. Only call from one thread at a time.
 */
class RadSim : appfw::NoMove {
public:
    using ProgressCallback = std::function<void(double progress)>;

    RadSim(int threadCount = -1);
    ~RadSim();

    //! Sets app config used by the rad.
    void setAppConfig(AppConfig *appcfg);

    //! This callback will be called to update progress for long tasks.
    //! Called from the same thread that called methods bellow.
    //! On end called with value of 1 (maybe multiple times).
    void setProgressCallback(const ProgressCallback &cb);

    //! Returns level light is calculated for.
    const bsp::Level *getLevel();

    //! Sets and loads the level.
    void setLevel(const bsp::Level *pLevel, const std::string &name,
                  const std::string &profileName);

    //! Returns the build profile.
    const BuildProfile &getBuildProfile();

    //! Sets the bounce count.
    void setBounceCount(int count);

    //! Returns hash of the level with this particular configuration of patches (position, count,
    //! etc). Valid after loadLevelConfig.
    appfw::SHA256::Digest getPatchHash();

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

private:
    std::unique_ptr<RadSimImpl> m_Impl;
};

} // namespace rad

#endif
