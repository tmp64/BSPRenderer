#include <rad/rad_config.h>

void rad::RadConfig::loadFromAppConfig(AppConfig &cfg) {
    auto rad = cfg.getItem("rad");
    flGamma = rad.get<float>("gamma");
    flRefl = rad.get<float>("reflectivity");
    flBaseRefl = rad.get<float>("base_reflectivity");
    flEnvLightDiv = rad.get<float>("light_env_brightness_divisor");
}
