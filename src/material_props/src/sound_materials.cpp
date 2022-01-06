#include <fstream>
#include <appfw/str_utils.h>
#include <material_props/sound_materials.h>

static const char s_TypeChars[(int)SoundMaterials::Type::TypeCount] = {
	0, // Undefined
    'M', // Metal
    'V', // Vent
    'D', // Dirt
    'S', // SloshLiquid
    'T', // Tile
    'G', // Grate
    'W', // Wood
    'P', // Computer
    'Y', // Glass
};

static const std::string_view s_TypeNames[(int)SoundMaterials::Type::TypeCount] = {
    "default",      // Undefined
    "metal",        // Metal
    "vent",         // Vent
    "dirt",         // Dirt
    "slosh_liquid", // SloshLiquid
    "tile",         // Tile
    "grate",        // Grate
    "wood",         // Wood
    "computer",     // Computer
    "glass",        // Glass
};

void SoundMaterials::clear() {
    m_Materials.clear();
}

void SoundMaterials::loadTextFile(const fs::path &path) {
    std::ifstream file(path);
    std::string line;
    
    while (std::getline(file, line)) {
        size_t pos = 0;

        // Skip spaces
        while (pos < line.size() && appfw::isAsciiSpace(line[pos])) {
            pos++;
        }

        if (pos == line.size()) {
            continue;
        }

        // Skip comments
        if (line[pos] == '/' && line[pos + 1] == '/') {
            continue;
        }

        // Get type
        char typeChar = appfw::charToUpper(line[pos]);
        Type type = Type::Undefined;
        for (int i = 1; i < (int)Type::TypeCount; i++) {
            if (s_TypeChars[i] == typeChar) {
                type = (Type)i;
                break;
            }
        }

        // Handle unknown types
        if (type == Type::Undefined) {
            continue;
        }

        pos++;

        // Skip spaces
        while (pos < line.size() && appfw::isAsciiSpace(line[pos])) {
            pos++;
        }

        if (pos == line.size()) {
            continue;
        }

        // Read texture name
        std::string texname = line.substr(pos, MAX_MATERIAL_LEN);
        appfw::strToLower(texname.begin(), texname.end());

        // Add to the map
        m_Materials[texname] = type;
    }
}

SoundMaterials::Type SoundMaterials::findMaterial(std::string_view name) const {
    std::string s(name.substr(0, MAX_MATERIAL_LEN));
    appfw::strToLower(s.begin(), s.end());
    auto it = m_Materials.find(s);

    if (it == m_Materials.end()) {
        return Type::Undefined;
    } else {
        return m_Materials.find(s)->second;
    }
}

std::string_view SoundMaterials::getTypeName(Type type) const {
    return s_TypeNames[(int)type];
}
