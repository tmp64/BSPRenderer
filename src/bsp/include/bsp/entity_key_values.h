#ifndef BSP_ENTITY_KEY_VALUES_H
#define BSP_ENTITY_KEY_VALUES_H
#include <vector>
#include <string>
#include <string_view>
#include <glm/glm.hpp>

namespace bsp {

//! A value of an entitiy's parameter.
class EntityValue {
public:
    EntityValue() = default;
    EntityValue(const std::string &value) { m_Value = value; }

    const std::string &asString() const { return m_Value; }
    const int asInt() const { return std::stoi(m_Value, nullptr, 10); }
    const float asFloat() const { return std::stof(m_Value, nullptr); }
    const glm::ivec3 asInt3() const;
    const glm::ivec4 asInt4() const;
    const glm::vec3 asFloat3() const;
    const glm::vec4 asFloat4() const; 

    void setString(std::string_view str) { m_Value = str; }

private:
    std::string m_Value;
};

//! Key-value pair of an entity's parameter.
class EntityKeyValues {
public:
    //! @returns the index of a key or -1 if not found.
    int indexOf(std::string_view key) const;

    //! @returns count of key-value pairs.
    inline int size() const { return (int)m_Keys.size(); }

    //! @returns key name of key idx (must be valid).
    inline std::string_view getKeyName(int idx) const { return m_Keys[idx].key; }

    //! @returns a value with specified key. If not found, throws std::invalid_argument.
    EntityValue &get(std::string_view key);

    //! @returns a value with specified key. If not found, throws std::invalid_argument.
    const EntityValue &get(std::string_view key) const;

    //! @returns a value with specified key index. If index is invalid,
    //! throws std::invalid_argument.
    EntityValue &get(int keyIdx);

    //! @returns a value with specified key index. If index is invalid,
    //! throws std::invalid_argument.
    const EntityValue &get(int keyIdx) const;

    //! @returns classname of the entity or empty string.
    std::string getClassName() const;

    //! @returns targetname of the entity or empty string.
    std::string getTargetName() const;

    //! Adds a new empty key to the key values.
    //! @returns index of the new key.
    int addNewKey(std::string_view name);

    //! Sets key's value to a string.
    void setString(std::string_view key, std::string_view str);

    //! Sets key's value to a formatted string.
    template <typename... Args>
    inline void setFormat(std::string_view key, std::string_view format, const Args &...args) {
        setString(key, fmt::make_format_args(args...));
    }

private:
    struct KeyValue {
        std::string key;
        EntityValue value;
    };

    int m_iTargetName = -1;
    int m_iClassName = -1;

    std::vector<KeyValue> m_Keys;
};

//! A collection of entities and their parameters.
class EntityKeyValuesDict {
public:
    EntityKeyValuesDict() = default;
    EntityKeyValuesDict(std::string_view entityLump);

    //! Loads entities from the entity lump string.
    //! On parse error, throws std::runtime_error.
    void loadFromString(std::string_view entityLump);

    //! Exports current entities to a string.
    std::string exportToString() const;

    //! @returns whether the list is empty.
    inline bool empty() const { return m_Ents.empty(); }

    //! @returns entity count
    inline int size() const { return (int)m_Ents.size(); }

    //! @returns entity by index (must be valid).
    inline EntityKeyValues &operator[](int idx) { return m_Ents[idx]; }

    //! @returns entity by index (must be valid).
    inline const EntityKeyValues &operator[](int idx) const { return m_Ents[idx]; }

    //! Finds first entity with "targetname" = targetname after entity with index "after".
    //! @returns index or -1
    int findEntityByName(std::string_view targetName, int after = -1) const;

    //! Finds first entity with "classname" = className after entity with index "after".
    //! @returns index or -1
    int findEntityByClassName(std::string_view className, int after = -1) const;

    inline auto begin() { return m_Ents.begin(); }
    inline auto begin() const { return m_Ents.begin(); }
    inline auto end() { return m_Ents.end(); }
    inline auto end() const { return m_Ents.end(); }

private:
    std::vector<EntityKeyValues> m_Ents;
};

} // namespace bsp 

#endif
