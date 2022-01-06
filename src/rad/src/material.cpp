#include "material.h"

void rad::Material::loadProps(MaterialPropLoader &loader, std::string_view texName,
                              std::string_view wadName) {
    m_Props = loader.loadProperties(texName, wadName);
}
