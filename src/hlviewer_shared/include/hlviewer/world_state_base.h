#ifndef HLVIEWER_WORLD_STATE_BASE_H
#define HLVIEWER_WORLD_STATE_BASE_H
#include <vector>
#include <memory>
#include <appfw/utils.h>
#include <bsp/level.h>
#include <renderer/const.h>
#include <hlviewer/assets/level_asset.h>
#include <hlviewer/entities/base_entity.h>
#include <hlviewer/brush_model.h>
#include <hlviewer/vis.h>
#include <hlviewer/level_view_renderer_iface.h>

class WorldStateBase : appfw::NoMove {
public:
    WorldStateBase(const bsp::Level &level, ILevelViewRenderer *pRenderer);
    ~WorldStateBase();

    virtual void tick(float flTimeDelta);

    //! Returns the level.
    inline const bsp::Level &getLevel() { return m_Level; }

    //! @returns the Vis instance
    inline Vis &getVis() { return m_Vis; }

    //! @returns the sky name
    std::string getSkyName();

    //! Returns the brush model or nullptr if doesn't exist.
    BrushModel *getBrushModel(size_t idx);

    //! Returns the list of all entities.
    inline auto &getEntList() { return m_EntityList; }

    //! @returns entity at index or nullptr.
    BaseEntity *getEntity(int entIdx);

    //! Creates a new entity.
    virtual BaseEntity *createEntity(std::string_view className, bsp::EntityKeyValues *kv = nullptr);

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
    const bsp::Level &m_Level;

    bsp::EntityKeyValuesDict m_LevelEntityDict;
    std::vector<std::unique_ptr<BaseEntity>> m_EntityList;

    std::vector<BrushModel> m_BrushModels;
    Vis m_Vis;
    ILevelViewRenderer *m_pViewRenderer = nullptr;

    // Timings
    float m_flTime = 0;
    float m_flTimeDelta = 0;

    // Lighting
    LightStyle m_LightStyles[MAX_LIGHTSTYLES];

    // Level loading
    void loadBrushModels();
    void loadEntities();

    void initLightStyles();
    void animateLights();
};

#endif
