#include <SDL.h>
#include <app_base/app_base.h>
#include <gui_app_base/config_component.h>

ConfigComponent::ConfigComponent(const std::string &configPath) {
    m_ConfigPath = configPath;
    setTickEnabled(true);
}

ConfigComponent::~ConfigComponent() {
    try {
        saveConfig();
    } catch (const std::exception &e) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Config save failed", e.what(), nullptr);
    }
}

void ConfigComponent::resetConfig() {
    auto &list = getItemList();

    for (ConfigItemBase *item : list) {
        item->resetDefaultValue();
    }

    requestSave();
}

void ConfigComponent::reloadConfig() {
    fs::path path = getFileSystem().findExistingFile(m_ConfigPath, std::nothrow);

    if (path.empty()) {
        resetConfig();
        return;
    }

    try {
        std::ifstream cfg(path);
        YAML::Node rootNode = YAML::Load(cfg);

        auto &list = getItemList();

        for (ConfigItemBase *item : list) {
            YAML::Node node = rootNode[std::string(item->getName())];

            if (node) {
                item->setYamlValue(node);
            } else {
                item->resetDefaultValue();
            }
        }
    } catch (const std::exception &e) {
        throw std::runtime_error(
            fmt::format("Failed to load config file {}:\n{}", m_ConfigPath, e.what()));
    }
}

void ConfigComponent::saveConfig() {
    try {
        YAML::Node rootNode;
        auto &list = getItemList();

        for (ConfigItemBase *item : list) {
            rootNode[std::string(item->getName())] = item->getYamlValue();
        }

        std::ofstream cfg(getFileSystem().getFilePath(m_ConfigPath));
        cfg << rootNode << "\n";
        m_flNextSaveTime = -1;
    } catch (const std::exception &e) {
        throw std::runtime_error(
            fmt::format("Failed to save config file {}:\n{}", m_ConfigPath, e.what()));
    }
}

void ConfigComponent::requestSave() {
    if (m_flNextSaveTime == -1) {
        m_flNextSaveTime = AppBase::getBaseInstance().getTime() + MIN_SAVE_PERIOD;
    }
}

void ConfigComponent::lateInit() {
    reloadConfig();
}

void ConfigComponent::tick() {
    if (m_flNextSaveTime != -1 && m_flNextSaveTime <= AppBase::getBaseInstance().getTime()) {
        saveConfig();
    }
}
