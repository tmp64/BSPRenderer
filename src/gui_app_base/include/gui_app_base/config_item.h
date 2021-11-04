#ifndef GUI_APP_BASE_CONFIG_ITEM_H
#define GUI_APP_BASE_CONFIG_ITEM_H
#include <appfw/utils.h>
#include <yaml-cpp/yaml.h>

class ConfigComponent;

class ConfigItemBase : appfw::NoMove {
public:
    //! @param  name    Unique name of the config item
    //! @param  descr   Description
    ConfigItemBase(std::string_view name, std::string_view descr);

    inline std::string_view getName() { return m_Name; }
    inline std::string_view getDescr() { return m_Descr; }

    //! Sets the value to a default one.
    virtual void resetDefaultValue() = 0;

    //! Returns the string value.
    virtual YAML::Node getYamlValue() = 0;

    //! Sets a string value.
    //! @return Success or failure
    virtual bool setYamlValue(const YAML::Node &yaml) = 0;

private:
    std::string_view m_Name;
    std::string_view m_Descr;

    //! Returns the list of all items
    static std::list<ConfigItemBase *> &getList();

    friend class ConfigComponent;
};

template <typename T>
class ConfigItem : public ConfigItemBase {
public:
    using Callback = std::function<bool(const T &oldVal, const T &newVal)>;
    using CallbackJustVal = std::function<void(const T &newVal)>;

    ConfigItem(std::string_view name, const T &defValue, std::string_view descr,
               Callback cb = Callback());
    ConfigItem(std::string_view name, const T &defValue, std::string_view descr,
               CallbackJustVal cb);

    //! Returns the value of the convar.
    inline const T &getValue() { return m_Value; }
    
    //! Returns the default value of the convar.
    inline const T &getDefaultValue() { return m_DefVal; }

    //! Sets a new value of the cvar.
    //! @return Whether or not new value was applied.
    bool setValue(const T &newVal);

    //! Sets a new default value of the cvar. Can be called before lateInit. Doesn't change actual
    //! value.
    //! @return Whether or not new value was applied.
    void setDefaultValue(const T &newVal);

    //! Sets the value changed callback.
    void setCallback(const Callback &callback);
    void setCallback(const CallbackJustVal &callback);

    void resetDefaultValue() override;
    YAML::Node getYamlValue() override;
    bool setYamlValue(const YAML::Node &yaml) override;

private:
    T m_Value;
    T m_DefVal;
    Callback m_Callback;
};

#endif
