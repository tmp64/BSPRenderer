#include <material_props/material_prop_loader.h>

void MaterialPropLoader::init(std::string_view mapName, std::string_view soundMatsFile,
                              std::string_view materialsDir) {
    m_MapName = mapName;
    m_MaterialsDir = materialsDir;

    fs::path soundMats = getFileSystem().findExistingFile(soundMatsFile, std::nothrow);

    if (!soundMats.empty()) {
        m_SndMats.loadTextFile(soundMats);
    } else {
        printe("Sound material file {} not found", soundMatsFile);
    }
}

std::string MaterialPropLoader::getBaseYamlPath(std::string_view baseMatName) {
    return fmt::format("{}/base/{}.yaml", m_MaterialsDir, baseMatName);
}

std::string MaterialPropLoader::getWadYamlPath(std::string_view textureName,
                                               std::string_view wadName) {
    return fmt::format("{}/wads/{}/{}.yaml", m_MaterialsDir, wadName, textureName);
}

std::string MaterialPropLoader::getMapYamlPath(std::string_view textureName) {
    return fmt::format("{}/maps/{}/{}.yaml", m_MaterialsDir, m_MapName, textureName);
}
