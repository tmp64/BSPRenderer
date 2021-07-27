#ifndef RAD_RAD_CONFIG_H
#define RAD_RAD_CONFIG_H
#include <app_base/app_config.h>
#include <glm/glm.hpp>

namespace rad {

struct RadConfig {
    //! Gamma for gamma correction
    float flGamma = 0;

    //! Reflectivity multiplier
    float flRefl = 0;

    //! Base surface reflectivity
    float flBaseRefl = 0;

    //! Divisor of default light_environment brightness
    float flEnvLightDiv = 0;

    //! Brightness multiplier relative to sun brightness
    float flSkyLightBrightness = 0;

    //! Loads the default config.
    void loadFromAppConfig(AppConfig &cfg);
};

} // namespace rad

#endif
