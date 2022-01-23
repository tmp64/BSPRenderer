#include "base_entity.h"
#include "world_state.h"

BaseEntity::BaseEntity() {
    updateAABB();
}

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
        setTargetName(value.asString());
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
        try {
            setFxColor(value.asInt3());
        } catch (const std::invalid_argument &) {
            // Some old maps (e.g. the HL campaign) have color set to "0"
            setFxColor(glm::ivec3(value.asInt()));
        }
        return true;
    } else if (key == "spawnflags") {
        m_uSpawnFlags = value.asInt();
        return true;
    }

    return false;
}

void BaseEntity::onKeyValuesUpdated() {}

void BaseEntity::showInspector() {
    ImGui::PushID("BaseEntity");
    if (ImGui::CollapsingHeader("Entity", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("%s", getClassName().c_str());
        ImGui::Text("Name: %s", getTargetName().c_str());

        ImGui::BeginDisabled();

        glm::vec3 origin = getOrigin();
        if (ImGui::InputFloat3("Origin", &origin.x, "%.1f")) {
            setOrigin(origin);
        }

        glm::vec3 angles = getAngles();
        if (ImGui::InputFloat3("Angles", &angles.x, "%.1f")) {
            setAngles(angles);
        }

        ImGui::EndDisabled();
    }
    ImGui::PopID();
}

void BaseEntity::setTargetName(std::string_view str) {
    m_TargetName = str;
}

void BaseEntity::setOrigin(glm::vec3 origin) {
    m_vOrigin = origin;
    updateAABB();
}

void BaseEntity::setAngles(glm::vec3 angles) {
    m_vAngles = angles;
    updateAABB();
}

void BaseEntity::updateSpawnFlags(unsigned flags) {
    m_uSpawnFlags = flags;
}

void BaseEntity::setUseAABB(bool state) {
    m_bUseAABB = state;
    updateAABB();
}

void BaseEntity::setAABBPos(glm::vec3 pos) {
    m_BaseAABBPos = pos;
    updateAABB();
}

void BaseEntity::setAABBHalfExtents(glm::vec3 halfExtents) {
    m_BaseAABBHalfExtents = halfExtents;
    updateAABB();
}

void BaseEntity::setAABBColor(glm::ivec3 color) {
    m_BaseAABBColor = color;
}

void BaseEntity::setAABBTintColor(glm::vec4 color) {
    m_AABBTintColor = color;
}

void BaseEntity::setModel(std::string_view model) {
    if (model[0] == '*') {
        // Brush model
        int modelidx = std::stoi(std::string(model.substr(1)), nullptr, 10);
        BrushModel *pModel = WorldState::get()->getBrushModel(modelidx);

        if (!pModel) {
            printe("Entity {}: invalid brush model index '{}'", entindex(), model);
        }

        setModel(pModel);
        setUseAABB(false); // Use brush model BSP tree instead
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

glm::quat BaseEntity::anglesToQuat(glm::vec3 angles) {
    glm::quat quat = glm::identity<glm::quat>();
    quat = glm::rotate(quat, glm::radians(angles.z), {1.0f, 0.0f, 0.0f});
    quat = glm::rotate(quat, glm::radians(angles.x), {0.0f, 1.0f, 0.0f});
    quat = glm::rotate(quat, glm::radians(angles.y), {0.0f, 0.0f, 1.0f});
    return quat;
}

void BaseEntity::updateAABB() {
    if (m_bUseAABB) {
        glm::vec3 pos = m_BaseAABBPos;
        glm::vec3 size = m_BaseAABBHalfExtents;
        glm::vec3 corners[] = {
            pos + glm::vec3(size.x, -size.y, -size.z), pos + glm::vec3(-size.x, -size.y, -size.z),
            pos + glm::vec3(-size.x, -size.y, size.z), pos + glm::vec3(size.x, -size.y, size.z),
            pos + glm::vec3(size.x, size.y, -size.z),  pos + glm::vec3(-size.x, size.y, -size.z),
            pos + glm::vec3(-size.x, size.y, size.z),  pos + glm::vec3(size.x, size.y, size.z),
        };

        glm::quat quat = anglesToQuat(getAngles());
        glm::vec3 mins = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 maxs = glm::vec3(std::numeric_limits<float>::lowest());

        for (const glm::vec3 &i : corners) {
            glm::vec3 v = quat * i;

            for (int j = 0; j < 3; j++) {
                mins[j] = std::min(mins[j], v[j]);
                maxs[j] = std::max(maxs[j], v[j]);
            }
        }

        m_AABBPos = (maxs + mins) / 2.0f;
        m_AABBHalfExtents = maxs - m_AABBPos;
    }
}
