#ifndef RAD_VISMAT_H
#define RAD_VISMAT_H
#include <vector>
#include <appfw/thread_pool.h>
#include <appfw/services.h>
#include <appfw/sha256.h>
#include <rad/types.h>

namespace rad {

class RadSim;

class VisMat {
public:
    static constexpr char VISMAT_MAGIC[] = "VISMAT001";

    VisMat(RadSim *pRadSim);

    /**
     * Returns true if a vismat is loaded.
     */
    bool isLoaded();

    /**
     * Returns true if a valid vismat is loaded.
     */
    bool isValid();

    /**
     * Returns current size of vismat in bytes.
     */
    size_t getCurrentSize();

    /**
     * Returns size of vismat in bytes that is need for a valid vismat.
     * This will be the size of vismat after buildVisMat.
     */
    size_t getValidSize();

    /**
     * Invalidates current vismat and loads a new one from a file.
     */
    void loadFromFile(const fs::path &path);

    /**
     * Saves visibility matrix to a file.
     */
    void saveToFile(const fs::path &path);

    /**
     * Builds a new vismat.
     */
    void buildVisMat();

    /**
     * Checks if p1 can see p2.
     */
    bool checkVisBit(PatchIndex p1, PatchIndex p2);

private:
    RadSim *m_pRadSim;
    bool m_bIsLoaded = false;
    appfw::SHA256::Digest m_PatchHash = {};
    std::vector<uint8_t> m_Data;

    void buildVisLeaves(appfw::ThreadPool::ThreadInfo &ti);
    void buildVisRow(PatchIndex patchnum, uint8_t *pvs, size_t bitpos, std::vector<uint8_t> &face_tested);
    void testPatchToFace(PatchIndex patchnum, int facenum, size_t bitpos);

    void decompressVis(const uint8_t *in, uint8_t *decompressed);
};

} // namespace rad

#endif
