#include "base_entity.h"
#include "world_state.h"

BaseEntity::BaseEntity() {}

void BaseEntity::loadKeyValues(const bsp::EntityKeyValues &item, int idx) {
    m_ClassName = item.getClassName();

    if (m_ClassName.empty()) {
        printe("Entity {}: no classname set", idx);
        m_ClassName = "< none >";
    }

    int kvModelIdx = item.indexOf("model");
    if (kvModelIdx != -1) {
        std::string model = item.get(kvModelIdx).asString();
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

    int kvOriginIdx = item.indexOf("origin");
    if (kvOriginIdx != -1) {
        std::string originstr = item.get(kvOriginIdx).asString();
        glm::vec3 origin;

        if (sscanf(originstr.c_str(), "%f %f %f", &origin.x, &origin.y, &origin.z) != 3) {
            printe("Entity {}: invalid origin string '{}'", idx, originstr);
        }

        m_vOrigin = origin;
    }

    int kvAnglesIdx = item.indexOf("angles");
    if (kvAnglesIdx != -1) {
        std::string anglesstr = item.get(kvAnglesIdx).asString();
        glm::vec3 angles;

        if (sscanf(anglesstr.c_str(), "%f %f %f", &angles.x, &angles.y, &angles.z) != 3) {
            printe("Entity {}: invalid angles string '{}'", idx, anglesstr);
        }

        m_vAngles = angles;
    }

    m_iRenderMode = getKVInt(item, "rendermode", 0);
    m_iRenderFx = getKVInt(item, "renderfx", 0);
    m_iFxAmount = getKVInt(item, "renderamt", 0);

    int kvRendercolorIdx = item.indexOf("rendercolor");
    if (kvRendercolorIdx != -1) {
        std::string colorstr = item.get(kvRendercolorIdx).asString();
        glm::ivec3 color;

        if (sscanf(colorstr.c_str(), "%d %d %d", &color.x, &color.y, &color.z) != 3) {
            printe("Entity {}: invalid rendercolor string '{}'", idx, colorstr);
        }

        m_vFxColor = color;
    }

    if (m_ClassName.substr(0, 8) == "trigger_" || m_ClassName == "func_ladder") {
        m_iRenderMode = kRenderTransTexture;
        m_iFxAmount = 127;
    }
}
