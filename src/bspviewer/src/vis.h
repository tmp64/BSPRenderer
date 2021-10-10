#ifndef VIS_H
#define VIS_H
#include <appfw/utils.h>
#include <bsp/level.h>

class Vis : appfw::NoMove {
public:
    static inline Vis &get() { return *m_spInstance; }
    Vis();
    ~Vis();

    //! Sets the level
    void setLevel(bsp::Level *pLevel);

    //! Returns whether AABB (mins, maxs) may be visible from point origin based on PVS data.
    bool boxInPvs(const glm::vec3 &origin, const glm::vec3 &mins, const glm::vec3 &maxs);

private:
    static constexpr int MAX_BOX_LEAFS = 256;

    struct LeafList {
        int count;
        int maxcount;
        bool overflowed;
        short *list;
        glm::vec3 mins, maxs;
        int topnode; // for overflows where each leaf can't be stored individually
    };

    bsp::Level *m_pLevel = nullptr;
    std::array<uint8_t, bsp::MAX_MAP_LEAFS / 8> m_VisBuf;

    //! Returns true if AABB is visible in VIS.
    bool isBoxVisible(const glm::vec3 &mins, const glm::vec3 &maxs, const uint8_t *visbits);

    //! Fills specified list with leafs in which AABB is located.
    //! @returns number of list items
    int boxLeafNums(const glm::vec3 &mins, const glm::vec3 &maxs, appfw::span<short> list);
    void boxLeafNums_r(LeafList &ll, int node);

    static inline Vis *m_spInstance = nullptr;
};

#endif
