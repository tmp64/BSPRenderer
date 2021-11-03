#include <gui_app_base/config_item.h>
#include <gui_app_base/config_component.h>

ConfigItemBase::ConfigItemBase(std::string_view name, std::string_view descr) {
    m_Name = name;
    m_Descr = descr;

    getList().push_back(this);
}

std::list<ConfigItemBase *> &ConfigItemBase::getList() {
    static std::list<ConfigItemBase *> list;
    return list;
}

template <typename T>
inline ConfigItem<T>::ConfigItem(std::string_view name, const T &defValue, std::string_view descr,
                                 Callback cb)
    : ConfigItemBase(name, descr) {
    m_Value = m_DefVal = defValue;
    setCallback(cb);
}

template <typename T>
ConfigItem<T>::ConfigItem(std::string_view name, const T &defValue, std::string_view descr,
                          CallbackJustVal cb)
    : ConfigItem(name, defValue, descr) {
    setCallback(cb);
}

template <typename T>
bool ConfigItem<T>::setValue(const T &newVal) {
    if (m_Callback) {
        if (!m_Callback(m_Value, newVal)) {
            return false;
        }
    }

    m_Value = newVal;
    ConfigComponent::get().requestSave();
    return true;
}

template <typename T>
void ConfigItem<T>::setCallback(const Callback &callback) {
    m_Callback = callback;
}

template <typename T>
void ConfigItem<T>::setCallback(const CallbackJustVal &callback) {
    setCallback([this, callback](const T &, const T &newVal) { return callback(newVal); });
}

template <typename T>
void ConfigItem<T>::resetDefaultValue() {
    setValue(m_DefVal);
}

template <typename T>
YAML::Node ConfigItem<T>::getYamlValue() {
    return YAML::Node(m_Value);
}

template <typename T>
bool ConfigItem<T>::setYamlValue(const YAML::Node &yaml) {
    return setValue(yaml.as<T>());
}

template class ConfigItem<bool>;
template class ConfigItem<int>;
template class ConfigItem<float>;
template class ConfigItem<double>;
template class ConfigItem<std::string>;