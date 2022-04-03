#include "world_state.h"
#include "main_view.h"
#include "bspviewer.h"

ConVar<std::string> sv_skyname("sv_skyname", "", "Current sky name");

WorldState::WorldState(const bsp::Level &level, ILevelViewRenderer *pRenderer)
    : WorldStateBase(level, pRenderer) {
    AFW_ASSERT(!m_spInstance);
    m_spInstance = this;

    m_MaterialLoader.init(BSPViewer::get().getMapName(), "assets:sound/materials.txt",
                          "assets:materials");

    sv_skyname.setCallback([&](const std::string &, const std::string &newVal) {
        MainView::get()->setSkyTexture(newVal);
        return true;
    });

    sv_skyname.setValue(getSkyName());
}

WorldState::~WorldState() {
    AFW_ASSERT(m_spInstance == this);
    m_spInstance = nullptr;
}

void WorldState::tick(float flTimeDelta) {
    WorldStateBase::tick(flTimeDelta);
}
