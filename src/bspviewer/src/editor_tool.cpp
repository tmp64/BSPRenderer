#include "editor_tool.h"

EditorTool::EditorTool(EditorMode *parentMode) {
    m_pMode = parentMode;
}

EditorTool::~EditorTool() {}

void EditorTool::onLevelLoaded() {}

void EditorTool::onLevelUnloaded() {}

void EditorTool::onActivated() {
    AFW_ASSERT(!m_bIsActive);
    m_bIsActive = true;
}

void EditorTool::onDeactivated() {
    AFW_ASSERT(m_bIsActive);
    m_bIsActive = false;
}

void EditorTool::onMainViewClicked(const glm::vec2 &) {}

void EditorTool::tick() {
    // Default action: do nothing
}

void EditorTool::setAlwaysTickEnabled(bool state) {
    m_bAlwaysTick = state;
}
