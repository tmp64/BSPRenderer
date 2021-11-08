#ifndef MATERIAL_PROPS_SOUND_MATERIAL_H
#define MATERIAL_PROPS_SOUND_MATERIAL_H
#include <map>
#include <appfw/filesystem.h>

class SoundMaterials {
public:
    enum class Type
    {
        Undefined,   //!< Not in the file
        Metal,       //!< 'M'
        Vent,        //!< 'V'
        Dirt,        //!< 'D'
        SloshLiquid, //!< 'S'
        Tile,        //!< 'T'
        Grate,       //!< 'G'
        Wood,        //!< 'W'
        Computer,    //!< 'P', obviously
        Glass,       //!< 'Y'
        TypeCount
    };

    //! Maximum length of material name.
    static constexpr size_t MAX_MATERIAL_LEN = 12;

    //! Clears previously loaded materials.
    void clear();

    //! Loads the materials from a materials.txt file. Doesn't clear, adds to existing ones instead.
    void loadTextFile(const fs::path &path);

    //! Finds a material type from name.
    Type findMaterial(std::string_view name) const;

    //! @returns the name of a type.
    std::string_view getTypeName(Type type) const;

private:
    std::map<std::string, Type> m_Materials;
};

#endif
