#ifndef SURFACE_EDITOR_FILES_H
#define SURFACE_EDITOR_FILES_H
#include <glm/glm.hpp>
#include <app_base/yaml.h>

constexpr size_t MAX_MAT_BASE_NAME = 32;

struct SurfacePropsFile {
    float flLightmapScale = 1.0f;
    float flLightIntensityScale = 1.0f;

    bool bOverrideLight = false;
    glm::vec3 vLightColor = glm::vec3(1, 1, 1);
    float flLightIntensity = 0.0f;

    bool bHasReflectivity = false;
    float flReflectivity = 0.5f;

    inline bool isIdentical(const SurfacePropsFile &other) {
        if (flLightmapScale != other.flLightmapScale) {
            return false;
        }

        if (flLightIntensityScale != other.flLightIntensityScale) {
            return false;
        }

        if (bOverrideLight != other.bOverrideLight) {
            return false;
        }

        if (bOverrideLight) {
            if (vLightColor != other.vLightColor) {
                return false;
            }

            if (flLightIntensity != other.flLightIntensity) {
                return false;
            }
        }

        if (bHasReflectivity != other.bHasReflectivity) {
            return false;
        }

        if (bHasReflectivity) {
            if (flReflectivity != other.flReflectivity) {
                return false;
            }
        }

        return true;
    }

    inline void loadFromYaml(const YAML::Node &node) {
        if (node["lightmap_scale"]) {
            flLightmapScale = node["lightmap_scale"].as<float>();
        }

        if (node["light_intensity_scale"]) {
            flLightIntensityScale = node["light_intensity_scale"].as<float>();
        }

        if (node["light_color"]) {
            bOverrideLight = true;
            vLightColor = node["light_color"].as<glm::vec3>() / 255.0f;
        }

        if (node["light_intensity"]) {
            bOverrideLight = true;
            flLightIntensity = node["light_intensity"].as<float>();
        }

        if (node["reflectivity"]) {
            bHasReflectivity = true;
            flReflectivity = node["reflectivity"].as<float>();
        }
    }

    inline void saveToYaml(YAML::Node &node) {
        node["lightmap_scale"] = flLightmapScale;
        node["light_intensity_scale"] = flLightIntensityScale;

        if (bOverrideLight) {
            node["light_color"] = vLightColor * 255.0f;
            node["light_intensity"] = flLightIntensity;
        }

        if (bHasReflectivity) {
            node["reflectivity"] = flReflectivity;
        }
    }
};

struct MaterialPropsFile {
    float flLightIntensityScale = 1.0f;

    bool bOverrideLight = false;
    glm::vec3 vLightColor = glm::vec3(1, 1, 1);
    float flLightIntensity = 0.0f;

    bool bHasReflectivity = false;
    float flReflectivity = 0.5f;

    char szBaseName[MAX_MAT_BASE_NAME] = "";

    inline MaterialPropsFile(bool hasBaseMat) { m_bHasBaseMat = hasBaseMat; }

    inline MaterialPropsFile getDefault() { return MaterialPropsFile(m_bHasBaseMat); }

    inline bool isIdentical(const MaterialPropsFile &other) {
        if (flLightIntensityScale != other.flLightIntensityScale) {
            return false;
        }

        if (bOverrideLight != other.bOverrideLight) {
            return false;
        }

        if (bOverrideLight) {
            if (vLightColor != other.vLightColor) {
                return false;
            }

            if (flLightIntensity != other.flLightIntensity) {
                return false;
            }
        }

        if (bHasReflectivity != other.bHasReflectivity) {
            return false;
        }

        if (bHasReflectivity) {
            if (flReflectivity != other.flReflectivity) {
                return false;
            }
        }

        if (m_bHasBaseMat && strcmp(szBaseName, other.szBaseName) != 0) {
            return false;
        }

        return true;
    }

    inline void loadFromYaml(YAML::Node &node) {
        if (node["light_intensity_scale"]) {
            flLightIntensityScale = node["light_intensity_scale"].as<float>();
        }

        if (node["light_color"]) {
            bOverrideLight = true;
            vLightColor = node["light_color"].as<glm::vec3>() / 255.0f;
        }

        if (node["light_intensity"]) {
            bOverrideLight = true;
            flLightIntensity = node["light_intensity"].as<float>();
        }

        if (node["reflectivity"]) {
            bHasReflectivity = true;
            flReflectivity = node["reflectivity"].as<float>();
        }

        if (node["base_material"]) {
            std::string mat = node["base_material"].as<std::string>();
            snprintf(szBaseName, sizeof(szBaseName), "%s", mat.c_str());
        }
    }

    inline void saveToYaml(YAML::Node &node) {
        node["light_intensity_scale"] = flLightIntensityScale;

        if (bOverrideLight) {
            node["light_color"] = vLightColor * 255.0f;
            node["light_intensity"] = flLightIntensity;
        }

        if (bHasReflectivity) {
            node["reflectivity"] = flReflectivity;
        }

        if (szBaseName[0]) {
            node["base_material"] = szBaseName;
        }
    }

private:
    bool m_bHasBaseMat = false;
};

struct BaseMaterialPropsFile {
    float flReflectivity = 0.5f;
    char szName[MAX_MAT_BASE_NAME] = "";

    inline BaseMaterialPropsFile getDefault() { return BaseMaterialPropsFile(); }

    inline bool isIdentical(const BaseMaterialPropsFile &other) {
        return flReflectivity == other.flReflectivity;
    }

    inline void loadFromYaml(YAML::Node &node) {
        if (node["reflectivity"]) {
            flReflectivity = node["reflectivity"].as<float>();
        }
    }

    inline void saveToYaml(YAML::Node &node) {
        node["reflectivity"] = flReflectivity;
    }
};

#endif
