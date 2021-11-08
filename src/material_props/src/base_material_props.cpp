#include <material_props/base_material_props.h>

void BaseMaterialProps::loadFromYaml(YAML::Node &node) {
    if (node["reflectivity"]) {
        flReflectivity = node["reflectivity"].as<float>();
    }
}
