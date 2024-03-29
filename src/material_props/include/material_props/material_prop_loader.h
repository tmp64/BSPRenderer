#ifndef MATERIAL_PROPS_MATERIAL_PROP_LOADER_H
#define MATERIAL_PROPS_MATERIAL_PROP_LOADER_H
#include <material_props/sound_materials.h>
#include <material_props/material_props.h>

//! Loads material props from base material, wad material and map material.
//! Base material is taken from wad material file or (if not set) from sound materials list.
class MaterialPropLoader {
public:
    //! Initializes the loader.
    //! @param  mapName         Name of the level
    //! @param  soundMatsFile   Virtual path to materials.txt
    //! @param  materialDir     Virtual path to material directory (without trailing slash).
    void init(std::string_view mapName, std::string_view soundMatsFile,
              std::string_view materialsDir);

    //! @returns path to base materials's YAML file.
    std::string getBaseYamlPath(std::string_view baseMatName);

    //! @returns path to wad materials's YAML file.
    std::string getWadYamlPath(std::string_view textureName, std::string_view wadName);

    //! @returns path to map materials's YAML file.
    std::string getMapYamlPath(std::string_view textureName);

    //! @returns SoundMaterials instance.
    inline SoundMaterials &getSoundMaterials() { return m_SndMats; }

    //! Loads properties for specified material texture name.
    MaterialProps loadProperties(std::string_view texName, std::string_view wadName);

    //! Loads a YAML file.
    //! @param  node    Node to load into
    //! @param  path    Virtual path
    //! @return whether the file exists or not
    static bool loadYaml(YAML::Node &node, std::string_view path);

private:
    SoundMaterials m_SndMats;
    std::string m_MapName;
    std::string m_MaterialsDir;
};

#endif
