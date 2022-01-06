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

MaterialProps MaterialPropLoader::loadProperties(std::string_view texName,
                                                 std::string_view wadName) {
    YAML::Node baseNode, wadNode, mapNode;

    std::string baseName;
    if (loadYaml(wadNode, getWadYamlPath(texName, wadName))) {
        if (wadNode["base_material"]) {
            baseName = wadNode["base_material"].as<std::string>();
        }
    }

    if (baseName.empty()) {
        SoundMaterials::Type type = m_SndMats.findMaterial(texName);
        baseName = m_SndMats.getTypeName(type);
    }

    if (!loadYaml(baseNode, getBaseYamlPath(baseName))) {
        printw("Base material '{}' not found", baseName);
    }

    loadYaml(mapNode, getMapYamlPath(texName));

    MaterialProps props;
    props.loadFromYaml(baseNode);
    props.loadFromYaml(wadNode);
    props.loadFromYaml(mapNode);
    return props;
}

bool MaterialPropLoader::loadYaml(YAML::Node &node, std::string_view vpath) {
    fs::path path = getFileSystem().findExistingFile(vpath, std::nothrow);

    if (path.empty()) {
        return false;
    }

    std::ifstream file(path);
    node = YAML::Load(file);
    return true;
}
