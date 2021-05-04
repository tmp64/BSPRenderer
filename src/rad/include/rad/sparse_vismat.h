#ifndef RAD_SPARSE_VISMAT_H
#define RAD_SPARSE_VISMAT_H
#include <appfw/services.h>
#include <appfw/sha256.h>
#include <appfw/thread_pool.h>
#include <rad/types.h>
#include <vector>

namespace rad {

class RadSim;

class SparseVisMat {
public:
    static constexpr char SVISMAT_MAGIC[] = "SVISMAT001";

    /**
     * One list item. All of it's contents are ones
     */
    struct ListItem {
        static constexpr uint16_t MAX_OFFSET = std::numeric_limits<uint16_t>::max();
        static constexpr uint16_t MAX_SIZE = std::numeric_limits<uint16_t>::max();

        uint16_t offset = 0; //!< Offset from previous block in bits (patches)
        uint16_t size = 0;   //!< Size of block in bits (patches). Can be zero (e.g. if previous offset == MAX_OFFSET).
    };

    SparseVisMat(RadSim *pRadSim);

    /**
     * Returns true if a vismat is loaded.
     */
    bool isLoaded();

    /**
     * Returns true if a valid vismat is loaded.
     */
    bool isValid();

    /**
     * Invalidates current vismat and loads a new one from a file.
     */
    void loadFromFile(const fs::path &path);

    /**
     * Saves visibility matrix to a file.
     */
    void saveToFile(const fs::path &path);

    /**
     * Creates a sparse matrix from a full one.
     */
    void buildSparseMat();

    /**
     * Unloads the matrix and frees all alloxated memory.
     */
    void unloadMatrix();

    inline const std::vector<size_t> &getOffsetTable() { return m_OffsetTable; }
    inline const std::vector<PatchIndex> &getCountTable() { return m_CountTable; }
    inline const std::vector<PatchIndex> &getOnesCountTable() { return m_OnesCountTable; }
    inline const std::vector<ListItem> &getListItems() { return m_ListItems; }
    inline size_t getTotalOnesCount() { return m_uTotalOnesCount; }

private:
    RadSim *m_pRadSim;
    bool m_bIsLoaded = false;
    appfw::SHA256::Digest m_PatchHash = {};
    std::vector<size_t> m_OffsetTable; //<! Offset into m_ListItems for each patch
    std::vector<PatchIndex> m_CountTable;     //!< Number of list items for each patch
    std::vector<PatchIndex> m_OnesCountTable; //!< Number of 1 bits for each patch
    std::vector<ListItem> m_ListItems;
    size_t m_uTotalOnesCount = 0;

    void calcSize();
    void compressMat();
    void validateMat();
};

}

#endif
