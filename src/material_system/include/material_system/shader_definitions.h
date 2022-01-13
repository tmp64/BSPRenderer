#ifndef MATERIAL_SYSTEM_SHADER_DEFINITIONS_H
#define MATERIAL_SYSTEM_SHADER_DEFINITIONS_H
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class ShaderStageDefinitions {
public:
    //! Adds a compile definition to the list
    //! @param  key     Name of the definition
    //! @param  value   Value of the definition as is (quotes are not added)
    void addDef(std::string_view key, std::string_view value = "");

    void addDef(std::string_view key, int value);
    void addDef(std::string_view key, unsigned value);
    void addDef(std::string_view key, float value);

    //! Converts the list into a string with #defines.
    std::string toString();

private:
    std::vector<std::pair<std::string, std::string>> m_Defs;
};

class ShaderProgramDefinitions {
public:
    template <typename T>
    void addSharedDef(std::string_view key, const T &value) {
        m_SharedDefs.addDef(key, value);
    }

    template <typename T>
    void addVertexDef(std::string_view key, const T &value) {
        m_VertexDefs.addDef(key, value);
    }

    template <typename T>
    void addFragmentDef(std::string_view key, const T &value) {
        m_FragmentDefs.addDef(key, value);
    }

    std::string toStringVertex() { return m_SharedDefs.toString() + m_VertexDefs.toString(); }
    std::string toStringFragment() { return m_SharedDefs.toString() + m_FragmentDefs.toString(); }

private:
    ShaderStageDefinitions m_SharedDefs;
    ShaderStageDefinitions m_VertexDefs;
    ShaderStageDefinitions m_FragmentDefs;
};

#endif
