#include <bsp/entity_key_values.h>

//-----------------------------------------------------------------
// EntityValue
//-----------------------------------------------------------------
const glm::ivec3 bsp::EntityValue::asInt3() const {
    glm::ivec3 vec(0, 0, 0);

    if (sscanf(m_Value.c_str(), "%d %d %d", &vec.x, &vec.y, &vec.z) != 3) {
        throw std::invalid_argument("value is not ivec3");
    }

    return vec;
}

const glm::ivec4 bsp::EntityValue::asInt4() const {
    glm::ivec4 vec(0, 0, 0, 0);

    if (sscanf(m_Value.c_str(), "%d %d %d %d", &vec.x, &vec.y, &vec.z, &vec.w) != 4) {
        throw std::invalid_argument("value is not ivec4");
    }

    return vec;
}

const glm::vec3 bsp::EntityValue::asFloat3() const {
    glm::vec3 vec(0, 0, 0);

    if (sscanf(m_Value.c_str(), "%f %f %f", &vec.x, &vec.y, &vec.z) != 3) {
        throw std::invalid_argument("value is not vec3");
    }

    return vec;
}

const glm::vec4 bsp::EntityValue::asFloat4() const {
    glm::vec4 vec(0, 0, 0, 0);

    if (sscanf(m_Value.c_str(), "%f %f %f %f", &vec.x, &vec.y, &vec.z, &vec.w) != 4) {
        throw std::invalid_argument("value is not vec4");
    }

    return vec;
}

//-----------------------------------------------------------------
// EntityKeyValues
//-----------------------------------------------------------------
int bsp::EntityKeyValues::indexOf(std::string_view key) const {
    for (int i = 0; i < m_Keys.size(); i++) {
        if (m_Keys[i].key == key) {
            return i;
        }
    }
    
    return -1;
}

bsp::EntityValue &bsp::EntityKeyValues::get(std::string_view key) {
    return get(indexOf(key));
}

const bsp::EntityValue &bsp::EntityKeyValues::get(std::string_view key) const {
    return get(indexOf(key));
}

bsp::EntityValue &bsp::EntityKeyValues::get(int keyIdx) {
    if (keyIdx < 0 || keyIdx > m_Keys.size()) {
        throw std::invalid_argument("keyIdx is invalid");
    }

    return m_Keys[keyIdx].value;
}

const bsp::EntityValue &bsp::EntityKeyValues::get(int keyIdx) const {
    if (keyIdx < 0 || keyIdx > m_Keys.size()) {
        throw std::invalid_argument("keyIdx is invalid");
    }

    return m_Keys[keyIdx].value;
}

std::string bsp::EntityKeyValues::getClassName() const {
    return m_iClassName != -1 ? m_Keys[m_iClassName].value.asString() : "";
}

std::string bsp::EntityKeyValues::getTargetName() const {
    return m_iTargetName != -1 ? m_Keys[m_iTargetName].value.asString() : "";
}

int bsp::EntityKeyValues::addNewKey(std::string_view name) {
    int idx = size();
    m_Keys.emplace_back();
    m_Keys[idx].key = name;

    if (name == "targetname") {
        m_iTargetName = idx;
    } else if (name == "classname") {
        m_iClassName = idx;
    }

    return idx;
}

void bsp::EntityKeyValues::setString(std::string_view key, std::string_view str) {
    int idx = indexOf(key);

    if (idx == -1) {
        idx = addNewKey(key);
    }

    get(idx).setString(str);
}

//-----------------------------------------------------------------
// EntityKeyValuesDict
//-----------------------------------------------------------------
bsp::EntityKeyValuesDict::EntityKeyValuesDict(std::string_view entityLump) {
    loadFromString(entityLump);
}

void bsp::EntityKeyValuesDict::loadFromString(std::string_view entityLump) {
    m_Ents.clear();
    size_t i = 0;

    auto fnSkipWhitespace = [&]() {
        auto &c = entityLump;
        while (i < entityLump.size() && (c[i] == ' ' || c[i] == '\r' || c[i] == '\n' ||
               c[i] == '\t')) {
            i++;
        }
    };

    auto fnCheckEOF = [&]() {
        if (i >= entityLump.size()) {
            throw std::runtime_error("unexpected end of file at position " + std::to_string(i));
        }
    };

    while (i < entityLump.size()) {
        fnSkipWhitespace();

        if (i == entityLump.size() || entityLump[i] == '\0') {
            // Finished parsing
            break;
        }

        if (entityLump[i] != '{') {
            throw std::runtime_error("expected '{' at position " + std::to_string(i));
        }
        i++;

        EntityKeyValues item;

        while (i < entityLump.size()) {
            fnSkipWhitespace();

            // "key"
            if (entityLump[i] != '}' && entityLump[i] != '"') {
                throw std::runtime_error("expected '\"' or '}' inside entity at position " +
                                         std::to_string(i));
            }

            if (entityLump[i] == '}') {
                break;
            }

            i++;
            size_t keyBegin = i;

            while (entityLump[i] != '"' && i < entityLump.size()) {
                i++;
            }

            fnCheckEOF();
            size_t keyEnd = i;
            i++;

            // "value"
            fnSkipWhitespace();
            if (entityLump[i] != '"') {
                throw std::runtime_error("expected '\"' at position " + std::to_string(i));
            }

            i++;
            size_t valueBegin = i;

            while (entityLump[i] != '"' && i < entityLump.size()) {
                i++;
            }

            fnCheckEOF();
            size_t valueEnd = i;
            i++;

            // Add into the item
            std::string_view key = entityLump.substr(keyBegin, keyEnd - keyBegin);
            std::string_view value = entityLump.substr(valueBegin, valueEnd - valueBegin);
            int keyIdx = item.addNewKey(key);
            item.get(keyIdx).setString(value);
        }

        fnCheckEOF();

        if (entityLump[i] != '}') {
            throw std::runtime_error("expected '}' at position " + std::to_string(i));
        }
        i++;

        // Add item into the list
        m_Ents.push_back(std::move(item));
    }
}

std::string bsp::EntityKeyValuesDict::exportToString() const {
    std::string str;

    for (const EntityKeyValues &kv : m_Ents) {
        std::string ent;
        ent += "{\n";

        for (int i = 0; i < kv.size(); i++) {
            ent += fmt::format("\"{}\" \"{}\"\n");
        }

        ent += "}\n";
        str += ent;
    }

    return str;
}

int bsp::EntityKeyValuesDict::findEntityByName(std::string_view targetName, int after) const {
    for (int i = after; i < size(); i++) {
        if (m_Ents[i].getTargetName() == targetName) {
            return i;
        }
    }

    return -1;
}

int bsp::EntityKeyValuesDict::findEntityByClassName(std::string_view className, int after) const {
    for (int i = after; i < size(); i++) {
        if (m_Ents[i].getClassName() == className) {
            return i;
        }
    }

    return -1;
}
