#ifndef RENDERER_SHADER_DEFINITIONS_H
#define RENDERER_SHADER_DEFINITIONS_H
#include <vector>
#include <utility>
#include <string>
#include <string_view>

class ShaderDefinitions {
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

#endif
