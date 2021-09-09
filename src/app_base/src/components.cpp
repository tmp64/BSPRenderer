#include <appfw/appfw.h>
#include <app_base/components.h>
#include <app_base/app_base.h>

AppFilesystemComponent::AppFilesystemComponent() {
    m_BasePath = app_getInitInfo().baseAppPath;

    if (m_BasePath.empty()) {
        m_BasePath = fs::current_path();
    }

    printi("Base app path: {}", m_BasePath.u8string());
    getFileSystem().addSearchPath(m_BasePath, "base");
}

AppConfigComponent::AppConfigComponent() {
    printi("Application: {}", app_getInitInfo().appDirName);
    std::string vpath = "base:" + app_getInitInfo().appDirName + "/app_config.yaml";
    fs::path path = getFileSystem().findExistingFile(vpath, std::nothrow);

    if (path.empty()) {
        app_fatalError("App config file not found: '{}'", vpath);
    }

    m_Config.loadYamlFile(path);
    m_Config.mountFilesystem();
    m_Config.executeCommands();
}
