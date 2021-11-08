#include <material_props/material_props.h>

void MaterialProps::loadFromYaml(YAML::Node &node) {
    if (node["reflectivity"]) {
        flReflectivity = node["reflectivity"].as<float>();
    }

    if (node["light_color"]) {
        vLightColor = node["light_color"].as<glm::vec3>() / 255.0f;
    }

    if (node["light_intensity"]) {
        flLightIntensity = node["light_intensity"].as<float>();
    }

    if (node["light_intensity_scale"]) {
        flLightIntensityScale *= node["light_intensity_scale"].as<float>();
    }
}