#ifndef MATERIAL_PROPS_BASE_MATERIAL_PROPS_H
#define MATERIAL_PROPS_BASE_MATERIAL_PROPS_H
#include <glm/glm.hpp>
#include <app_base/yaml.h>

struct BaseMaterialProps {
    float flReflectivity = 0.5f;

    void loadFromYaml(YAML::Node &node);
};

#endif
