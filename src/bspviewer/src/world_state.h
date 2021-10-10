#ifndef WORLD_STATE_H
#define WORLD_STATE_H
#include <vector>
#include <memory>
#include <appfw/utils.h>
#include <bsp/level.h>
#include "assets/level_asset.h"
#include "base_entity.h"
#include "brush_model.h"
#include "vis.h"

class WorldState : appfw::NoMove {
public:
    static inline WorldState &get() { return *m_spInstance; }

    WorldState(LevelAssetRef level);
    ~WorldState();

    //! Returns the level.
    inline const bsp::Level &getLevel() { return m_pLevelAsset->getLevel(); }

    //! Returns the brush model or nullptr if doesn't exist.
    BrushModel *getBrushModel(size_t idx);

    //! Returns the list of all entities.
    inline auto &getEntList() { return m_EntityList; }

private:
    // Level info
    LevelAssetRef m_pLevelAsset;
    bsp::Level *m_pLevel;
    std::vector<std::unique_ptr<BaseEntity>> m_EntityList;
    std::vector<BrushModel> m_BrushModels;
    Vis m_Vis;

    // Level loading
    void loadBrushModels();
    void loadEntities();
    void optimizeBrushModels();

    static inline WorldState *m_spInstance = nullptr;
};

#endif
