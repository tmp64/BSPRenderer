#ifndef APP_BASE_COMPONENTS_H
#define APP_BASE_COMPONENTS_H
#include <appfw/filesystem.h>
#include <app_base/app_component.h>
#include <app_base/app_config.h>

//! Loads base app path and adds it to "base" search group
class AppFilesystemComponent : public AppComponentBase<AppFilesystemComponent> {
public:
    AppFilesystemComponent();

private:
    fs::path m_BasePath;
};

//! Loads the app config
class AppConfigComponent : public AppComponentBase<AppConfigComponent> {
public:
    AppConfigComponent();
    inline AppConfig &getConfig() { return m_Config; }

private:
    AppConfig m_Config;
};

#endif
