#include "light_entity.h"
#include "world_state.h"

void LightEntity::spawn() {
    setAABBColor({250, 159, 47});
    setAABBHalfExtents(glm::vec3(4));

    if (isSpawnFlagSet(SF_LIGHT_START_OFF)) {
        disableLight();
    } else {
        enableLight();
    }
}

bool LightEntity::parseKeyValue(const std::string &key, const bsp::EntityValue &value) {
    if (key == "style") {
        m_iStyle = value.asInt();
        return true;
    } else if (key == "pattern") {
        m_Pattern = value.asString();
        return true;
    } else {
        return BaseEntity::parseKeyValue(key, value);
    }
}

void LightEntity::showInspector() {
    ImGui::PushID("LightEntity");
    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Lightstyle: %d", m_iStyle);
        ImGui::Text("Pattern: %s", m_Pattern.empty() ? "(none)" : m_Pattern.c_str());

        if (!isToggleable()) {
            ImGui::Text("Non-toggleable");
        } else {
            ImGui::Text("Status: %s", m_bEnabled ? "On" : "Off");

            if (ImGui::Button("Enable")) {
                enableLight();
            }

            ImGui::SameLine();

            if (ImGui::Button("Disable")) {
                disableLight();
            }
        }
    }
    ImGui::PopID();
}

void LightEntity::enableLight() {
    if (isToggleable()) {
        if (!m_Pattern.empty()) {
            WorldState::get()->setLightStyle(m_iStyle, m_Pattern.c_str());
        } else {
            WorldState::get()->setLightStyle(m_iStyle, "m");
        }

        m_bEnabled = true;
    }
}

void LightEntity::disableLight() {
    if (isToggleable()) {
        WorldState::get()->setLightStyle(m_iStyle, "a");
        m_bEnabled = false;
    }
}
