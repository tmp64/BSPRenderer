#include <fstream>
#include <nlohmann/json.hpp>
#include <appfw/appfw.h>
#include <app_base/app_config.h>

//------------------------------------------------------
// Item
//------------------------------------------------------
AppConfig::Item::Item(nlohmann::json *pJson) { m_pValue = pJson; }

template <typename T>
T AppConfig::Item::get(const std::string &key, const std::optional<T> &defVal) const {
    auto it = m_pValue->find(key);

    if (it == m_pValue->end()) {
        if (defVal.has_value()) {
            return *defVal;
        } else {
            throw std::invalid_argument("key " + key + " not found");
        }
    } else {
        return it->get<T>();
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

AppConfig::Item AppConfig::Item::getSubItem(const std::string &key) const { return Item(&m_pValue->at(key)); }

const nlohmann::json &AppConfig::Item::getJson() const { return *m_pValue; }

bool AppConfig::Item::contains(const std::string &key) const { return m_pValue->contains(key); }

//------------------------------------------------------
// AppConfig
//------------------------------------------------------
AppConfig::AppConfig(const fs::path &path) {
    try {
        loadJsonFile(path);
    } catch (const std::exception &e) {
        throw std::runtime_error(fmt::format("AppConfig: {}", e.what()));
    }
}

bool AppConfig::itemExists(const std::string &key) {
    auto it = m_pData->find(key);
    return it != m_pData->end();
}

AppConfig::Item AppConfig::getItem(const std::string &key) { return Item(&m_pData->at(key)); }

void AppConfig::mountFilesystem() {
    Item fs = getItem("filesystem");

    for (auto &item : fs.getJson().get<nlohmann::json::object_t>()) {
        const nlohmann::json &array = item.second.get<nlohmann::json>();

        for (auto &i : array) {
            const std::string &pathStr = i.get<std::string>();
            fs::path path = fs::u8path(pathStr);

            if (path.is_absolute()) {
                getFileSystem().addSearchPath(path, item.first.c_str());
            } else {
                fs::path p = getFileSystem().findExistingFile("base:" + pathStr);
                getFileSystem().addSearchPath(p, item.first.c_str());
            }
        }
    }
}

void AppConfig::executeCommands() {
    if (!itemExists("commands")) {
        return;
    }

    auto &item = getItem("commands").getJson();

    if (!item.is_array()) {
        throw std::logic_error("AppConfig: commands is not an array");
    }

    for (auto &i : item) {
        appfw::getConsole().command(i.get<std::string>());
    }
}

void AppConfig::loadJsonFile(const fs::path &path) {
    using nlohmann::json;
    
    std::ifstream file(path);

    if (file.fail()) {
        throw std::runtime_error(fmt::format("failed to open: {}", strerror(errno)));
    }

    std::vector<char> fileData;
    appfw::readFileContents(file, fileData);
    fileData.push_back('\0');
    json obj = json::parse(fileData.data(), nullptr, true, true);

    if (!obj.is_object()) {
        throw std::runtime_error("JSON file is not an object");
    }

    m_pData = std::make_shared<nlohmann::json>(std::move(obj));
}

