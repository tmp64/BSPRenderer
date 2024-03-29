#ifndef GUI_APP_APP_CONFIG_H
#define GUI_APP_APP_CONFIG_H
#include <optional>
#include <memory>
#include <map>
#include <yaml-cpp/yaml.h>
#include <appfw/filesystem.h>

class AppConfig {
public:
    /**
     * A reference to a JSON object.
     */
    class Item {
    public:
        /**
         * Construct the item from a JSON value.
         */
        Item(YAML::Node &node);

        /**
         * Returns value for a key. If not found or invalid type, throws an exception.
         */
        template <typename T>
        T get(const std::string &key, const std::optional<T> &defVal = std::nullopt) const;

        /**
         * Returns a subitem. If not found, throws an exception.
         */
        Item getSubItem(const std::string &key) const;

        /**
         * Returns a raw YAML node.
         */
        inline const YAML::Node &getYamlNode() const { return m_Node; }

        /**
         * Returns whether `key` exists.
         */
        bool contains(const std::string &key) const;

    private:
        YAML::Node m_Node;
    };

    /**
     * Constructs empty AppConfig.
     */
    AppConfig() = default;

    /**
     * Constructs AppConfig, loads base and app-specific configs.
     */
    AppConfig(const fs::path &path);

    /**
     * Loads a YAML object file.
     */
    void loadYamlFile(const fs::path &path);

    /**
     * Returns whether item exists in the root.
     */
    bool itemExists(const std::string &key);

    /**
     * Returns an item with specified key.
     */
    Item getItem(const std::string &key);

    /**
     * Mounts filesystems listed in "filesystem" object.
     */
    void mountFilesystem();

    /**
     * Executes commands listed in "commands" array.
     */
    void executeCommands();

private:
    YAML::Node m_Data;
};

#endif
