#ifndef RAD_LEVEL_CONFIG_H
#define RAD_LEVEL_CONFIG_H
#include <appfw/appfw.h>
#include <glm/glm.hpp>
#include <app_base/app_config.h>
#include <app_base/yaml.h>

namespace rad {

struct RadConfig;

struct SunLight {
    float flPitch = 0;
    float flYaw = 0;
    glm::vec3 vColor = glm::vec3(0, 0, 0); //!< Color in gamma space
    float flBrightness = 0;
    bool bIsSet = false;
};

struct LevelConfig {
    //! YAML document of the config
    YAML::Node yaml;

    //! Gamma for gamma correction
    float flGamma = 0;

    //! Reflectivity multiplier
    float flRefl = 0;

    //! Direct sunlight.
    SunLight sunLight;

    //! Loads the default config.
    void loadDefaults(const RadConfig &cfg);

    //! Loads the level config
    void loadLevelConfig(const fs::path &path);
};

struct BuildProfile {
    //! Base size of a patch in hammer units.
    int iBasePatchSize = -1;

    //! Minimum allowed patch size after subdivision.
    float flMinPatchSize = -1;

    //! Size of a lightmap pixel.
    float flLuxelSize = -1;

    //! Number of light bounce passes.
    int iBounceCount = -1;

    //! Loads the profile from a YAML document
    void loadProfile(const YAML::Node &node);

    //! Validates that all options are set. Throws if not.
    //! Sets luxel size to patch size if it's 0.
    void finalize();
};

} // namespace rad

#endif
