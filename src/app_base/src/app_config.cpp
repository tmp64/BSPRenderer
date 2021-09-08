#include <fstream>
#include <appfw/appfw.h>
#include <app_base/app_config.h>

//------------------------------------------------------
// Item
//------------------------------------------------------
AppConfig::Item::Item(YAML::Node &node) {
    m_Node = node;
}

template <typename T>
T AppConfig::Item::get(const std::string &key, const std::optional<T> &defVal) const {
    auto node = m_Node[key];

    if (node) {
        return node.as<T>();
    } else {
        if (defVal.has_value()) {
            return *defVal;
        } else {
            throw std::invalid_argument("key " + key + " not found");
        }
    }
}

template
int AppConfig::Item::get(const std::string &key, const std::optional<int> &defVal) const;

template
float AppConfig::Item::get(const std::string &key, const std::optional<float> &defVal) const;

template
double AppConfig::Item::get(const std::string &key, const std::optional<double> &defVal) const;

template
bool AppConfig::Item::get(const std::string &key, const std::optional<bool> &defVal) const;

template
std::string AppConfig::Item::get(const std::string &key, const std::optional<std::string> &defVal) const;

AppConfig::Item AppConfig::Item::getSubItem(const std::string &key) const {
    YAML::Node node = m_Node[key];

    if (!node) {
        throw std::invalid_argument("key " + key + " not found");
    }

    return Item(node);
}

bool AppConfig::Item::contains(const std::string &key) const {
    return m_Node[key].IsDefined();
}

//------------------------------------------------------
// AppConfig
//------------------------------------------------------
AppConfig::AppConfig(const fs::path &path) {
    try {
        loadYamlFile(path);
    } catch (const std::exception &e) {
        throw std::runtime_error(fmt::format("AppConfig: {}", e.what()));
    }
}

bool AppConfig::itemExists(const std::string &key) {
    return m_Data[key].IsDefined();
}

AppConfig::Item AppConfig::getItem(const std::string &key) {
    YAML::Node node = m_Data[key];

    if (!node) {
        throw std::invalid_argument("key " + key + " not found");
    }

    return Item(node);
}

void AppConfig::mountFilesystem() {
    Item fs = getItem("filesystem");
    const YAML::Node &fsNode = fs.getYamlNode();

    if (fsNode.IsNull()) {
        return;
    }

    if (!fsNode.IsMap()) {
        throw std::logic_error("AppConfig: 'filesystem' is not a map");
    }

    for (auto &item : fsNode) {
        const std::string tag = item.first.as<std::string>();

        if (!item.second.IsSequence()) {
            throw std::logic_error(fmt::format("AppConfig: 'filesystem.{}' is not a list",
                                               item.first.as<std::string>()));
        }

        std::list<std::string> paths = item.second.as<std::list<std::string>>();

        for (auto &pathStr : paths) {
            fs::path path = fs::u8path(pathStr);

            if (path.is_absolute()) {
                getFileSystem().addSearchPath(path, tag);
            } else {
                fs::path p = getFileSystem().findExistingFile("base:" + pathStr);
                getFileSystem().addSearchPath(p, tag);
            }
        }
    }
}

void AppConfig::executeCommands() {
    if (!itemExists("commands")) {
        return;
    }

    auto &item = getItem("commands").getYamlNode();

    if (!item.IsSequence()) {
        throw std::logic_error("AppConfig: 'commands' is not a list");
    }

    std::list<std::string> cmds = item.as<std::list<std::string>>();

    for (auto &i : cmds) {
        appfw::getConsole().command(i);
    }
}

void AppConfig::loadYamlFile(const fs::path &path) {
    std::ifstream file(path);

    if (file.fail()) {
        throw std::runtime_error(fmt::format("failed to open: {}", strerror(errno)));
    }

    m_Data = YAML::Load(file);
}

