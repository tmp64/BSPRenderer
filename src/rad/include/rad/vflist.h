#ifndef RAD_VFLIST_H
#define RAD_VFLIST_H
#include <appfw/services.h>
#include <appfw/sha256.h>
#include <appfw/thread_pool.h>
#include <rad/types.h>

namespace rad {

class RadSim;
class PatchRef;

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

    inline const std::vector<size_t> &getPatchOffsets() { return m_Offsets; }
    inline const std::vector<float> &getVFData() { return m_Data; }
    inline const std::vector<float> &getVFKoeff() { return m_Koeff; }

private:
    RadSim *m_pRadSim;
    bool m_bIsLoaded = false;
    appfw::SHA256::Digest m_PatchHash = {};
    std::vector<size_t> m_Offsets;
    std::vector<float> m_Data;
    std::vector<float> m_Koeff; //!< Value that you need to multiply vf with to get vf for patch i.

    /**
     * Calculates offsets into m_Data for each patch.
     */
    void calcOffsets();

    /**
     * Calculates view factors between patches
     */
    void calcViewFactors();

    /**
     * View factor worker.
     */
    void worker(appfw::ThreadPool::ThreadInfo &ti);

    /**
     * Fills m_Sum[i] with sum of all viewfactors of patch i.
     */
    void sumViewFactors();

    /**
     * Calculates view factor between patches.
     * Assumes patches can see each other.
     */
    float calcPatchViewfactor(PatchRef &patch1, PatchRef &patch2);

#if 0
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
#endif
};

} // namespace rad

#endif
