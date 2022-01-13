#include <fmt/format.h>
#include <material_system/shader_definitions.h>

void ShaderStageDefinitions::addDef(std::string_view key, std::string_view value) {
    m_Defs.push_back({std::string(key), std::string(value)});
}

void ShaderStageDefinitions::addDef(std::string_view key, int value) {
    addDef(key, fmt::format("{}", value));
}

void ShaderStageDefinitions::addDef(std::string_view key, unsigned value) {
    addDef(key, fmt::format("{}", value));
}

void ShaderStageDefinitions::addDef(std::string_view key, float value) {
    addDef(key, fmt::format("{:e}", value));
}

std::string ShaderStageDefinitions::toString() {
    std::string str;

    for (auto &i : m_Defs) {
        str.append(fmt::format("#define {} {}\n", i.first, i.second));
    }

    return str;
}
