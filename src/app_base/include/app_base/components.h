#ifndef APP_BASE_COMPONENTS_H
#define APP_BASE_COMPONENTS_H
#include <appfw/filesystem.h>
#include <appfw/extcon/extcon_host.h>
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

//! Enables extcon if '--extcon-port' arg is present
class AppExtconComponent : public AppComponentBase<AppExtconComponent> {
public:
    AppExtconComponent();
    ~AppExtconComponent();

    void tick() override;
    void requestClientFocus();
    inline bool isExtconEnabled() { return m_Host.isEnabled(); }
    inline bool isExtconConnected() { return m_Host.isConnected(); }
    inline bool isHostFocusRequested() { return m_bFocusRequested; }

private:
    appfw::ExtconHost m_Host;
    bool m_bFocusRequested = false;
};

#endif
