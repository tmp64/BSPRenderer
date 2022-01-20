#include <input/key_bind.h>
#include "entity_select_tool.h"
#include "main_view_renderer.h"
#include "vis.h"
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

    auto &mainView = MainViewRenderer::get();
    bool drawEntities = mainView.isEntityRenderingEnabled();
    Ray ray = mainView.screenPointToRay(position);
    EntityRaycastHit hit;

    if (drawEntities) {
        Vis::get().raycastToEntity(ray, hit);
    }

    if (hit.entity != -1) {
        printw("Hit {} - {}", hit.entity,
               WorldState::get()->getEntList()[hit.entity]->getClassName());
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

    if (pEnt->getModel() && pEnt->getModel()->getType() == ModelType::Brush) {
        BrushModel &model = static_cast<BrushModel &>(*pEnt->getModel());

        int firstFace = model.getFirstFace();
        int lastFace = firstFace + model.getFaceNum();

        for (int i = firstFace; i < lastFace; i++) {
            MainViewRenderer::get().setSurfaceTint(i, SELECTION_COLOR);
        }
    } else {
        pEnt->setAABBTintColor(SELECTION_COLOR);
    }
}

void EntitySelectTool::clearTinting() {
    BaseEntity *pEnt = WorldState::get()->getEntity(m_iEntIdx);

    if (pEnt->getModel() && pEnt->getModel()->getType() == ModelType::Brush) {
        BrushModel &model = static_cast<BrushModel &>(*pEnt->getModel());

        int firstFace = model.getFirstFace();
        int lastFace = firstFace + model.getFaceNum();

        for (int i = firstFace; i < lastFace; i++) {
            MainViewRenderer::get().setSurfaceTint(i, glm::vec4(0));
        }
    } else {
        pEnt->setAABBTintColor(glm::vec4(0));
    }
}
