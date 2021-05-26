#ifndef WORLD_STATE_H
#define WORLD_STATE_H
#include <vector>
#include <memory>
#include <appfw/utils.h>
#include <bsp/level.h>
#include "base_entity.h"
#include "brush_model.h"

class WorldState : appfw::NoMove {
public:
    static inline WorldState &get() { return *m_spInstance; }

    WorldState();
    ~WorldState();

    inline bool isLoaded() { return m_pLevel != nullptr; }
    void loadLevel(const bsp::Level &level);
    inline const bsp::Level &getLevel() { return *m_pLevel; }
    BrushModel *getBrushModel(size_t idx);

    inline auto &getEntList() { return m_EntityList; }

    //!< Called when renderer is fully loaded
    void onRendererReady();

    /**
     * Returns whether AABB (mins, maxs) is visible from point origin based on PVS data.
     */
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

    const bsp::Level *m_pLevel = nullptr;
    std::vector<std::unique_ptr<BaseEntity>> m_EntityList;
    std::vector<BrushModel> m_BrushModels;
    std::array<uint8_t, bsp::MAX_MAP_LEAFS / 8> m_VisBuf;

    void loadBrushModels();
    void loadEntities();

    /**
     * Returns true if AABB is visible in VIS.
     */
    bool isBoxVisible(const glm::vec3 &mins, const glm::vec3 &maxs, const uint8_t *visbits);

    /**
     * Fills specified list with leafs in which AABB is located.
     * @returns number of list items
     */
    int boxLeafNums(const glm::vec3 &mins, const glm::vec3 &maxs, appfw::span<short> list);

    void boxLeafNums_r(LeafList &ll, int node);

    static inline WorldState *m_spInstance = nullptr;
};

#endif
