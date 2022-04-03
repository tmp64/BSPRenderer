#include <input/key_bind.h>
#include <hlviewer/vis.h>
#include "entity_select_tool.h"
#include "main_view.h"
#include "world_state.h"

extern KeyBind key_clear_selection;

EntitySelectTool::EntitySelectTool(EditorMode *parentMode)
    : EditorTool(parentMode) {}

void EntitySelectTool::clearSelection() {
    if (m_iEntIdx != -1) {
        if (isActive()) {
            clearTinting();
        }
        m_iEntIdx = -1;
    }
}

void EntitySelectTool::setSelection(int entIdx) {
    clearSelection();

    m_iEntIdx = entIdx;

    if (isActive()) {
        showTinting();
    }
}

const char *EntitySelectTool::getName() {
    return "Select";
}

void EntitySelectTool::onLevelUnloaded() {
    EditorTool::onLevelUnloaded();
    clearSelection();
}

void EntitySelectTool::onActivated() {
    EditorTool::onActivated();

    if (m_iEntIdx != -1) {
        showTinting();
    }
}

void EntitySelectTool::onDeactivated() {
    EditorTool::onDeactivated();

    if (m_iEntIdx != -1) {
        clearTinting();
    }
}

void EntitySelectTool::onMainViewClicked(const glm::vec2 &position) {
    clearSelection();

    auto &mainView = *MainView::get();
    bool drawEntities = mainView.isEntityRenderingEnabled();
    Ray ray = mainView.viewportPointToRay(position);
    EntityRaycastHit hit;

    if (drawEntities) {
        WorldState::get()->getVis().raycastToEntity(ray, hit, !mainView.showTriggers());
    }

    if (hit.entity != -1) {
        setSelection(hit.entity);
    }
}

void EntitySelectTool::tick() {
    if (key_clear_selection.isPressed()) {
        clearSelection();
    }
}

void EntitySelectTool::showTinting() {
    BaseEntity *pEnt = WorldState::get()->getEntity(m_iEntIdx);

    if (pEnt->getModel() && pEnt->getModel()->type == ModelType::Brush) {
        BrushModel &model = static_cast<BrushModel &>(*pEnt->getModel());

        unsigned firstFace = model.uFirstFace;
        unsigned lastFace = firstFace + model.uFaceNum;

        for (unsigned i = firstFace; i < lastFace; i++) {
            MainView::get()->setSurfaceTint(i, SELECTION_COLOR);
        }
    } else {
        pEnt->setAABBTintColor(SELECTION_COLOR);
    }
}

void EntitySelectTool::clearTinting() {
    BaseEntity *pEnt = WorldState::get()->getEntity(m_iEntIdx);

    if (pEnt->getModel() && pEnt->getModel()->type == ModelType::Brush) {
        BrushModel &model = static_cast<BrushModel &>(*pEnt->getModel());

        unsigned firstFace = model.uFirstFace;
        unsigned lastFace = firstFace + model.uFaceNum;

        for (unsigned i = firstFace; i < lastFace; i++) {
            MainView::get()->setSurfaceTint(i, glm::vec4(0));
        }
    } else {
        pEnt->setAABBTintColor(glm::vec4(0));
    }
}
