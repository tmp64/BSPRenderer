#include <rad/rad_config.h>
#include <rad/level_config.h>

void rad::LevelConfig::loadDefaults(const RadConfig &cfg) {
    flGamma = cfg.flGamma;
    flRefl = cfg.flRefl;
}

void rad::LevelConfig::loadLevelConfig(const fs::path &path) {
    if (!fs::exists(path)) {
        // Default config will be used
        return;
    }

    std::ifstream file(path);

    if (file.fail()) {
        throw std::runtime_error(fmt::format("failed to open config: {}", strerror(errno)));
    }

    yaml = YAML::Load(file);

    if (yaml["gamma"]) {
        flGamma = yaml["gamma"].as<float>();
    }

    if (yaml["reflectivity"]) {
        flRefl = yaml["reflectivity"].as<float>();
    }

    if (yaml["sunlight"]) {
        auto n = yaml["sunlight"];
        SunLight &s = sunLight;
        s.flPitch = n["pitch"].as<float>();
        s.flYaw = n["yaw"].as<float>();
        s.vColor = n["color"].as<glm::vec3>() / 255.0f;
        s.flBrightness = n["brightness"].as<float>();
        s.bIsSet = true;
    }
}

void rad::BuildProfile::loadProfile(const YAML::Node &node) {
    if (node["base_patch_size"]) {
        iBasePatchSize = node["base_patch_size"].as<int>();
    }

    if (node["min_patch_size"]) {
        flMinPatchSize = node["min_patch_size"].as<float>();
    }

    if (node["luxel_size"]) {
        if (node["luxel_size"].IsNull()) {
            flLuxelSize = 0;
        } else {
            flLuxelSize = node["luxel_size"].as<float>();
        }
    }

    if (node["bounce_count"]) {
        iBounceCount = node["bounce_count"].as<int>();
    }
}

void rad::BuildProfile::finalize() {
    if (iBasePatchSize == -1) {
        throw std::runtime_error("Profile: base_patch_size not set. Check _common profile.");
    }

    if (flMinPatchSize == -1) {
        throw std::runtime_error("Profile: min_patch_size not set. Check _common profile.");
    }

    if (flLuxelSize == -1) {
        throw std::runtime_error("Profile: luxel_size not set. Check _common profile.");
    }

    if (iBounceCount == -1) {
        throw std::runtime_error("Profile: bounce_count not set. Check _common profile.");
    }

    if (flLuxelSize == 0) {
        flLuxelSize = (float)iBasePatchSize;
    }
}
