#ifndef GUI_APP_APP_CONFIG_H
#define GUI_APP_APP_CONFIG_H
#include <optional>
#include <memory>
#include <map>
#include <nlohmann/json_fwd.hpp>
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
        Item(nlohmann::json *pJson);

        
        // Getters for items. If not found or invalid type, throws an exception.
        /*int getInt(const std::string &key, const std::optional<int> &defVal = std::nullopt) const;
        float getFloat(const std::string &key, const std::optional<float> &defVal = std::nullopt) const;
        double getDouble(const std::string &key, const std::optional<double> &defVal = std::nullopt) const;
        bool getBool(const std::string &key, const std::optional<bool> &defVal = std::nullopt) const;
        std::string getString(const std::string &key, const std::optional<std::string> &defVal = std::nullopt) const;*/

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
         * Returns raw JSON value.
         */
        const nlohmann::json &getJson() const;

        /**
         * Returns whether `key` exists.
         */
        bool contains(const std::string &key) const;

    private:
        nlohmann::json *m_pValue = nullptr;
    };

    /**
     * Constructs AppConfig, loads base and app-specific configs.
     */
    AppConfig();

    /**
     * Returns an item with specified key.
     */
    Item getItem(const std::string &key);

private:
    std::shared_ptr<nlohmann::json> m_pData;

    /**
     * Loads a JSON object file.
     */
    void loadJsonFile(const fs::path &path);
};

#endif
