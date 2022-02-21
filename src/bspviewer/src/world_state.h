#ifndef WORLD_STATE_H
#define WORLD_STATE_H
#include <vector>
#include <memory>
#include <appfw/utils.h>
#include <bsp/level.h>
#include <material_props/material_prop_loader.h>
#include <renderer/const.h>
#include "assets/level_asset.h"
#include "entities/base_entity.h"
#include "brush_model.h"
#include "vis.h"

class WorldState : appfw::NoMove {
public:
    static inline WorldState *get() { return m_spInstance; }

    WorldState(LevelAssetRef level);
    ~WorldState();

    void tick();

    //! Returns the level.
    inline const bsp::Level &getLevel() { return m_pLevelAsset->getLevel(); }

    //! Returns the brush model or nullptr if doesn't exist.
    BrushModel *getBrushModel(size_t idx);

    //! Returns the list of all entities.
    inline auto &getEntList() { return m_EntityList; }

    //! @returns entity at index or nullptr.
    BaseEntity *getEntity(int entIdx);

    //! @returns the material properties loader.
    inline MaterialPropLoader &getMaterialLoader() { return m_MaterialLoader; }

    //! Creates a new entity.
    BaseEntity *createEntity(std::string_view className, bsp::EntityKeyValues *kv = nullptr);

    //! @returns the simulation time.
    inline float getTime() { return m_flTime; }

    //! @returns simulation time since last tick. Can return 0 if on pause.
    inline float getTimeDelta() { return m_flTimeDelta; }

    //! Update lightstyle animation.
    //! @param  style       Lightstyle index [0; MAX_LIGHTSTYLES).
    //! @param  pattern     Animation pattern of 'a' to 'z' (corresponds to x0 and x2 brightness).
    //! @param  animTime    Current animation time (seconds).
    void setLightStyle(int style, const char *pattern, float animTime = 0);

private:
    //! Light animation frequency (Hz).
    static constexpr float LIGHT_ANIM_FREQ = 10;

    struct LightStyle {
        //! Pattern of the light: 'a' to 'z'.
        //! 'a' = x0
        //! 'z' = x2
        char pattern[256] = "a";

        //! Length of the pattern.
        int patternLength = 0;

        //! Pattern converted to scale [0.0; 2.0].
        float scale[256] = {};

        //! Animation start time
        float startTime = 0;
    };

    // Level info
    LevelAssetRef m_pLevelAsset;
    bsp::Level *m_pLevel;

    bsp::EntityKeyValuesDict m_LevelEntityDict;
    std::vector<std::unique_ptr<BaseEntity>> m_EntityList;

    std::vector<BrushModel> m_BrushModels;
    Vis m_Vis;
    MaterialPropLoader m_MaterialLoader;

    // Timings
    float m_flTime = 0;
    float m_flTimeDelta = 0;

    // Lighting
    LightStyle m_LightStyles[MAX_LIGHTSTYLES];

    // Level loading
    void loadBrushModels();
    void loadEntities();
    void loadDefaultSky();

    void initLightStyles();
    void animateLights();

    static inline WorldState *m_spInstance = nullptr;
};

#endif
