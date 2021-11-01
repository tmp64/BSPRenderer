#include "base_entity.h"
#include "world_state.h"

BaseEntity::BaseEntity() {}

void BaseEntity::loadKeyValues(const bsp::Level::EntityListItem &item, int idx) {
    if (item.hasValue("model")) {
        std::string model = item.getValue<std::string>("model");
        if (model[0] == '*') {
            // Brush model
            int modelidx = std::stoi(model.substr(1), nullptr, 10);
            BrushModel *pModel = WorldState::get()->getBrushModel(modelidx);

            if (!pModel) {
                printe("Entity {}: invalid model index '{}'", idx, model);
            }

            m_pModel = pModel;
            m_vOrigin = pModel->m_vOrigin;
        }
    }

    if (item.hasValue("origin")) {
        std::string originstr = item.getValue<std::string>("origin");
        glm::vec3 origin;

        if (sscanf(originstr.c_str(), "%f %f %f", &origin.x, &origin.y, &origin.z) != 3) {
            printe("Entity {}: invalid origin string '{}'", idx, originstr);
        }

        m_vOrigin = origin;
    }

    if (item.hasValue("angles")) {
        std::string anglesstr = item.getValue<std::string>("angles");
        glm::vec3 angles;

        if (sscanf(anglesstr.c_str(), "%f %f %f", &angles.x, &angles.y, &angles.z) != 3) {
            printe("Entity {}: invalid angles string '{}'", idx, anglesstr);
        }

        m_vAngles = angles;
    }

    m_iRenderMode = item.getValue<int>("rendermode", 0);
    m_iRenderFx = item.getValue<int>("renderfx", 0);
    m_iFxAmount = item.getValue<int>("renderamt", 0);

    if (item.hasValue("rendercolor")) {
        std::string colorstr = item.getValue<std::string>("rendercolor");
        glm::ivec3 color;

        if (sscanf(colorstr.c_str(), "%d %d %d", &color.x, &color.y, &color.z) != 3) {
            printe("Entity {}: invalid rendercolor string '{}'", idx, colorstr);
        }

        m_vFxColor = color;
    }

    if (item.getValue<std::string>("classname").substr(0, 8) == "trigger_") {
        m_iRenderMode = kRenderTransTexture;
        m_iFxAmount = 127;
    }
}
