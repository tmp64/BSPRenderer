#include <input/key_bind.h>
#include <hlviewer/vis.h>
#include "surface_select_tool.h"
#include "main_view.h"
#include "world_state.h"

KeyBind key_clear_selection("Clear selection", KeyCode::D, KMOD_CTRL);

SurfaceSelectTool::SurfaceSelectTool(EditorMode *parentMode)
    : EditorTool(parentMode) {}

void SurfaceSelectTool::clearSelection() {
    if (m_iSurface != -1) {
        if (isActive()) {
            clearTinting();
        }

        m_iSurface = -1;
        m_iEntity = -1;
    }
}

void SurfaceSelectTool::setSelection(int surfIdx, int entIdx) {
    AFW_ASSERT_REL(surfIdx >= 0);
    clearSelection();

    m_iSurface = surfIdx;
    m_iEntity = entIdx;

    if (isActive()) {
        showTinting();
    }
}

const char *SurfaceSelectTool::getName() {
    return "Select";
}

void SurfaceSelectTool::onLevelUnloaded() {
    EditorTool::onLevelUnloaded();
    clearSelection();
}

void SurfaceSelectTool::onActivated() {
    EditorTool::onActivated();

    if (m_iSurface != -1) {
        showTinting();
    }
}

void SurfaceSelectTool::onDeactivated() {
    EditorTool::onDeactivated();

    if (m_iSurface != -1) {
        clearTinting();
    }
}

void SurfaceSelectTool::onMainViewClicked(const glm::vec2 &position) {
    clearSelection();

    auto &mainView = *MainView::get();
    auto &vis = WorldState::get()->getVis();
    bool drawWorld = mainView.isWorldRenderingEnabled();
    bool drawEntities = mainView.isEntityRenderingEnabled();
    Ray ray = mainView.viewportPointToRay(position);
    SurfaceRaycastHit hit;

    if (drawWorld && drawEntities) {
        vis.raycastToSurface(ray, hit, !mainView.showTriggers());
    } else if (drawWorld) {
        vis.raycastToWorldSurface(ray, hit);
    } else if (drawEntities) {
        vis.raycastToEntitySurface(ray, hit, !mainView.showTriggers());
    }

    if (hit.surface != -1) {
        setSelection(hit.surface, hit.entIndex);
    }
}

void SurfaceSelectTool::tick() {
    if (key_clear_selection.isPressed()) {
        clearSelection();
    }
}

void SurfaceSelectTool::showTinting() {
    MainView::get()->setSurfaceTint(m_iSurface, SELECTION_COLOR);
}

void SurfaceSelectTool::clearTinting() {
    MainView::get()->setSurfaceTint(m_iSurface, glm::vec4(0, 0, 0, 0));
}
