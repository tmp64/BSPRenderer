#ifndef RAD_VFLIST_H
#define RAD_VFLIST_H
#include <appfw/services.h>
#include <appfw/sha256.h>
#include <appfw/thread_pool.h>
#include <rad/types.h>

namespace rad {

class RadSim;

/**
 * View factor list
 */
class VFList {
public:
    static constexpr char VF_MAGIC[] = "VFLIST001";

    VFList(RadSim *pRadSim);

    /**
     * Returns whether vflist if loaded.
     */
    bool isLoaded();

    /**
     * Returns whether vflist if valid for loaded level and config.
     */
    bool isValid();

    /**
     * Invalidates current vflist and loads a new one from a file.
     */
    void loadFromFile(const fs::path &path);

    /**
     * Saves vflist to a file.
     */
    void saveToFile(const fs::path &path);

    /**
     * Calculates view factors between patches.
     * Requires a valid vismat.
     */
    void calculateVFList();

    /**
     * Return number of viewfactors of a patch.
     */
    inline PatchIndex getPatchVFCount(PatchIndex p) { return m_VFCount[p]; }

private:
    RadSim *m_pRadSim;
    bool m_bIsLoaded = false;
    appfw::SHA256::Digest m_PatchHash = {};
    std::vector<PatchIndex> m_VFCount;
    std::vector<ViewFactor> m_Data;

    /**
     * Calculates how many patch can any patch see.
     */
    void calcCount();

    /**
     * Sets m_Data pointers in the patches.
     */
    void setPointers();

    /**
     * Calculates view factors between patches 
     */
    void calcViewFactors();

    /**
     * View factor worker.
     */
    void worker(appfw::ThreadPool::ThreadInfo &ti);
};

} // namespace rad

#endif
