#include <imgui.h>
#include "editor_mode.h"
#include "editor_tool.h"

EditorMode::EditorMode() {}

EditorMode::~EditorMode() {}

void EditorMode::onLevelLoaded() {
    for (EditorTool *tool : m_Tools) {
        tool->onLevelLoaded();
    }
}

void EditorMode::onLevelUnloaded() {
    for (EditorTool *tool : m_Tools) {
        tool->onLevelLoaded();
    }
}

void EditorMode::onActivated() {
    AFW_ASSERT(!m_bIsActive);

    if (m_pActiveTool) {
        m_pActiveTool->onActivated();
    }

    m_bIsActive = true;
}

void EditorMode::onDeactivated() {
    AFW_ASSERT(m_bIsActive);

    if (m_pActiveTool) {
        m_pActiveTool->onDeactivated();
    }

    m_bIsActive = false;
}

void EditorMode::tick() {
    tickTools();
}

void EditorMode::showToolSelection() {
    for (size_t i = 0; i < m_Tools.size(); i++) {
        if (i > 0)
            ImGui::SameLine();
        if (ImGui::RadioButton(m_Tools[i]->getName(), m_Tools[i] == m_pActiveTool)) {
            activateTool(m_Tools[i]);
        }
    }
}

void EditorMode::showInspector() {}

void EditorMode::setAlwaysTickEnabled(bool state) {
    m_bAlwaysTick = state;
}

void EditorMode::registerTool(EditorTool *tool) {
    m_Tools.push_back(tool);
}

void EditorMode::tickTools() {
    for (EditorTool *tool : m_Tools) {
        AFW_ASSERT(tool->isActive() ? tool == m_pActiveTool : tool != m_pActiveTool);
        if (tool->isActive() || tool->isAlwaysTickEnabled()) {
            tool->tick();
        }
    }
}

void EditorMode::activateTool(EditorTool *tool) {
    if (m_pActiveTool && isActive()) {
        m_pActiveTool->onDeactivated();
    }

    m_pActiveTool = tool;

    if (tool && isActive()) {
        tool->onActivated();
    }
}
