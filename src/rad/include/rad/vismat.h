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
     * Builds a new vismat.
     */
    void buildVisMat();

    /**
     * Checks if p1 can see p2.
     */
    bool checkVisBit(PatchIndex p1, PatchIndex p2);

    /**
     * Frees all allocated memory.
     */
    void unloadVisMat();

    /**
     * Returns data vector.
     */
    inline const std::vector<uint8_t> &getData() { return m_Data; }

    /**
     * Returns offset of the row in bits.
     * Usage: bitpos = getRowOffset(p1) + p2.
     * p1 < p2 must be true
     */
    inline size_t getRowOffset(PatchIndex p1) { return m_Offsets[p1] - (size_t)p1 - 1; }

    /**
     * Returns bitpos of a vis between two patches.
     * p1 < p2 must be true
     */
    inline size_t getPatchBitPos(PatchIndex p1, PatchIndex p2) { return getRowOffset(p1) + (size_t)p2; }

private:
    //! Each vismat row will be aligned to this many bytes
    static constexpr size_t ROW_ALIGNMENT = sizeof(uint8_t);
    static constexpr size_t ROW_ALIGNMENT_BITS = ROW_ALIGNMENT * 8;

    RadSim *m_pRadSim;
    bool m_bIsLoaded = false;
    appfw::SHA256::Digest m_PatchHash = {};
    std::vector<size_t> m_Offsets;
    std::vector<uint8_t> m_Data;

    /**
     * Fills `offsets` with offsets to beginning of the row in bits.
     * @return size in bytes of the data array
     */
    size_t calculateOffsets(std::vector<size_t> &offsets);

    void buildVisLeaves(appfw::ThreadPool::ThreadInfo &ti);
    void buildVisRow(PatchIndex patchnum, uint8_t *pvs, size_t bitpos, std::vector<uint8_t> &face_tested);
    void testPatchToFace(PatchIndex patchnum, int facenum, size_t bitpos);

    void decompressVis(const uint8_t *in, uint8_t *decompressed);
};

} // namespace rad

#endif
