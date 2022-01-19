#include "base_entity.h"
#include "world_state.h"

BaseEntity::BaseEntity() {}

BaseEntity::~BaseEntity() {}

void BaseEntity::initialize(int entIdx, std::string_view className, bsp::EntityKeyValues *kv) {
    m_iEntIndex = entIdx;
    m_ClassName = className;

    if (kv) {
        m_pLevelKeyValues = kv;

        for (int i = 0; i < kv->size(); i++) {
            try {
                parseKeyValue(kv->getKeyName(i), kv->get(i));
            } catch (const std::exception &e) {
                printe("Entity {}: key-values error: {}", entindex(), e.what());
            }
        }
    }
    
    onKeyValuesUpdated();
}

void BaseEntity::spawn() {}

bool BaseEntity::parseKeyValue(const std::string &key, const bsp::EntityValue &value) {
    if (key == "targetname") {
        
        return true;
    } else if (key == "model") {
        setModel(value.asString());
        return true;
    } else if (key == "origin") {
        setOrigin(value.asFloat3());
        return true;
    } else if (key == "angles") {
        setAngles(value.asFloat3());
        return true;
    } else if (key == "rendermode") {
        setRenderMode((RenderMode)value.asInt());
        return true;
    } else if (key == "renderfx") {
        setRenderFx((RenderFx)value.asInt());
        return true;
    } else if (key == "renderamt") {
        setFxAmount(value.asInt());
        return true;
    } else if (key == "rendercolor") {
        setFxColor(value.asInt3());
        return true;
    }

    return false;
}

void BaseEntity::onKeyValuesUpdated() {}

void BaseEntity::setTargetName(std::string_view str) {
    m_TargetName = str;
}

void BaseEntity::setOrigin(glm::vec3 origin) {
    m_vOrigin = origin;
}

void BaseEntity::setAngles(glm::vec3 angles) {
    m_vAngles = angles;
}

void BaseEntity::setModel(std::string_view model) {
    if (model[0] == '*') {
        // Brush model
        int modelidx = std::stoi(std::string(model.substr(1)), nullptr, 10);
        BrushModel *pModel = WorldState::get()->getBrushModel(modelidx);

        if (!pModel) {
            printe("Entity {}: invalid brush model index '{}'", entindex(), model);
        }

        m_pModel = pModel;
        m_vOrigin = pModel->m_vOrigin;
    }
}

void BaseEntity::setModel(Model *model) {
    m_pModel = model;
}

void BaseEntity::setRenderMode(RenderMode mode) {
    m_nRenderMode = mode;
}

void BaseEntity::setRenderFx(RenderFx fx) {
    m_nRenderFx = fx;
}

void BaseEntity::setFxAmount(int amount) {
    m_iFxAmount = amount;
}

void BaseEntity::setFxColor(glm::ivec3 color) {
    m_vFxColor = color;
}
