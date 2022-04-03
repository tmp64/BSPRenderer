#ifndef WORLD_STATE_H
#define WORLD_STATE_H
#include <hlviewer/world_state_base.h>
#include <material_props/material_prop_loader.h>

class WorldState : public WorldStateBase {
public:
    static inline WorldState *get() { return m_spInstance; }

    WorldState(const bsp::Level &level, ILevelViewRenderer *pRenderer);
    ~WorldState();

    void tick(float flTimeDelta) override;

    //! @returns the material properties loader.
    inline MaterialPropLoader &getMaterialLoader() { return m_MaterialLoader; }

private:
    MaterialPropLoader m_MaterialLoader;

    static inline WorldState *m_spInstance = nullptr;
};

#endif
