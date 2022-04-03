#include <imgui.h>
#include "entity_edit_mode.h"
#include "world_state.h"
#include "bspviewer.h"

static constexpr float TEXTURE_PREVIEW_SIZE = 96.0f;

EntityEditMode::EntityEditMode()
    : m_SelectTool(this) {
    registerTool(&m_SelectTool);
    activateTool(&m_SelectTool);
}

const char *EntityEditMode::getName() {
    return "Entity";
}

void EntityEditMode::showInspector() {
    int entIdx = m_SelectTool.getEntityIndex();

    if (entIdx != -1) {
        BaseEntity *pEnt = WorldState::get()->getEntity(entIdx);
        pEnt->showInspector();

        bsp::EntityKeyValues *kv = pEnt->getKeyValues();

        if (kv) {
            ImGui::PushID("KeyValues");
            if (ImGui::CollapsingHeader("KeyValues", ImGuiTreeNodeFlags_DefaultOpen)) {
                int flags =
                    ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders;

                if (ImGui::BeginTable("table", 2, flags)) {
                    ImGui::TableSetupColumn("Key");
                    ImGui::TableSetupColumn("Value");
                    ImGui::TableHeadersRow();

                    for (int i = 0; i < kv->size(); i++) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(kv->getKeyName(i).c_str());
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(kv->get(i).asString().c_str());
                    }

                    ImGui::EndTable();
                }
            }
            ImGui::PopID();
        }
    } else {
        ImGui::Text("No entity selected.");
    }
}
