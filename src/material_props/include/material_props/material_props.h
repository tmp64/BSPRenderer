#ifndef MATERIAL_PROPS_MATERIAL_PROPS_H
#define MATERIAL_PROPS_MATERIAL_PROPS_H
#include <glm/glm.hpp>
#include <app_base/yaml.h>

struct MaterialProps {
    //! Reflectivity of the surface. It will be multiplied by the texture color.
    float flReflectivity = 0.5f;

    //! Light color in gamma-space
    glm::vec3 vLightColor = glm::vec3(1, 1, 1);

    //! Light intensity
    float flLightIntensity = 0.0f;

    //! Intensity scale. Map material files can change this to change intensity relative to the
    //! global material.
    float flLightIntensityScale = 1.0f;

    //! Loads the properties from a YAML node. They will be added to the existing ones.
    void loadFromYaml(YAML::Node &node);
};

#endif
