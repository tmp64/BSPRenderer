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

AppExtconComponent::AppExtconComponent() {
    int port = getCommandLine().getArgInt("--extcon-port", 0);

    if (port > 0 && port <= 65535) {
        setTickEnabled(true);
        appfw::getConsole().addConsoleReceiver(&m_Host);
        m_Host.enable(appfw::ADDR4_LOOPBACK, (uint16_t)port);
    }
}

AppExtconComponent::~AppExtconComponent() {
    if (m_Host.isEnabled()) {
        m_Host.disable();
        appfw::getConsole().removeConsoleReceiver(&m_Host);
    }
}

void AppExtconComponent::tick() {
    AFW_ASSERT(m_Host.isEnabled());
    m_Host.tick();
    m_bFocusRequested = m_Host.isHostFocusRequested();
}

void AppExtconComponent::requestClientFocus() {
    m_Host.requestCientFocus();
}
