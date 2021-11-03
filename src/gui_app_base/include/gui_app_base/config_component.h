#ifndef GUI_APP_BASE_CONFIG_COMPONENT_H
#define GUI_APP_BASE_CONFIG_COMPONENT_H
#include <app_base/app_component.h>
#include <gui_app_base/config_item.h>

class ConfigComponent : public AppComponentBase<ConfigComponent> {
public:
    static constexpr float MIN_SAVE_PERIOD = 3.0f;

    //! @param  configPath  Virtual path to the config.
    ConfigComponent(const std::string &configPath);
    ~ConfigComponent();

    //! Returns the list of all components
    inline const std::list<ConfigItemBase *> getItemList() { return ConfigItemBase::getList(); }

    //! Restores all values to default ones and requests a save.
    void resetConfig();

    //! Reloads the config from disk.
    void reloadConfig();

    //! Saves the current config.
    void saveConfig();

    //! Schedules a save a few seconds later. Allows changing multiple items without unnecessary
    //! config rewrites.
    void requestSave();

    void lateInit() override;
    void tick() override;

private:
    std::string m_ConfigPath;
    float m_flNextSaveTime = -1;
};

#endif
