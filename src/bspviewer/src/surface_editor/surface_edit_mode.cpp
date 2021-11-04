#include <imgui.h>
#include "surface_edit_mode.h"
#include "../world_state.h"

SurfaceEditMode::SurfaceEditMode()
    : m_SelectTool(this) {
    registerTool(&m_SelectTool);
    activateTool(&m_SelectTool);
}

const char *SurfaceEditMode::getName() {
    return "Surface";
}

void SurfaceEditMode::showInspector() {
    int surfIdx = m_SelectTool.getSurfaceIndex();
    int entIdx = m_SelectTool.getEntityIndex();

    if (surfIdx == -1) {
        ImGui::Text("No surface selected.");
    } else {
        ImGui::Text("Surface %d", surfIdx);

        if (entIdx == -1) {
            ImGui::Text("World");
        } else {
            const std::string &entClassName =
                WorldState::get()->getEntList()[entIdx]->getClassName();
            ImGui::Text("Entity %d (%s)", entIdx, entClassName.c_str());
        }
    }
}
