#ifndef RAD_MATERIAL_H
#define RAD_MATERIAL_H
#include <material_props/material_props.h>
#include <material_props/material_prop_loader.h>

namespace rad {

class Material {
public:
    //! Loads properties for the specified material.
    //! @param  loader  material prop loader
    //! @param  texName Name of the material
    //! @param  wadName Name of the WAD
    void loadProps(MaterialPropLoader &loader, std::string_view texName, std::string_view wadName);

    //! @returns the material properties.
    inline const MaterialProps &getProps() { return m_Props; }

private:
    MaterialProps m_Props;
};

} // namespace rad

#endif
