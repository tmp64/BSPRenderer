#include <appfw/console/std_console.h>
#include <appfw/log/console_logger.h>
#include <appfw/dbg.h>
#include <appfw/services.h>
#include <appfw/platform.h>
#include <appfw/init.h>

static appfw::console::ConsoleSystem *s_ConSys = nullptr;
static appfw::console::STDConsole *s_STDCon = nullptr;
static appfw::log::BaseLogger *s_Logger = nullptr;

appfw::console::ConsoleSystem &appfw::getConsole() { return *s_ConSys; }

appfw::log::BaseLogger &appfw::getLogger() { return *s_Logger; }

void appfw::init::init(InitOptions options) {
    if (options.bEnableConsole) {
        s_ConSys = new console::ConsoleSystem();
    }

    if (options.bRegisterItems) {
        getConsole().registerAllAvailableItems();
    }

    if (options.bEnableSTDConsole) {
        AFW_ASSERT_MSG(options.bEnableConsole, "Console is required");
        s_STDCon = new appfw::console::STDConsole();
        s_ConSys->addConsoleReceiver(s_STDCon);
    }

    if (options.bEnableLogger) {
        AFW_ASSERT_MSG(options.bEnableConsole, "Console is required");
        s_Logger = new appfw::log::ConsoleLogger(getConsole());
    }

    if (options.bPrintConfig) {
        AFW_ASSERT_MSG(options.bEnableLogger, "Logger is required");
        logInfo("APPFW Version 0.0.0");
        logInfo("Build date: {}", platform::getBuildDate());
        logInfo("OS and compiler: {}, {}", platform::getOSName(), platform::getCompilerName());

        if (platform::isDebugBuild()) {
            logInfo("Debug build");
        }

        logInfo("Configuration:");
        logInfo("\tConsole: {}", options.bEnableConsole ? "on" : "off");
        logInfo("\tSTDIO Console: {}", options.bEnableSTDConsole ? "on" : "off");
        logInfo("\tRegister all items: {}", options.bRegisterItems ? "on" : "off");
        logInfo("\tLogger: {}", options.bEnableLogger ? "on" : "off");
        logInfo("\tPrint config: {}", options.bPrintConfig ? "on" : "off");
    }
}

void appfw::init::mainLoopTick() {
    if (s_Logger) {
        s_Logger->processQueue();
    }

    if (s_STDCon) {
        s_STDCon->processQueue();
    }

    if (s_ConSys) {
        s_ConSys->processCommand();
    }
}

void appfw::init::shutdown() {
    logInfo("APPFW Shutting down\n");

    if (s_STDCon) {
        s_ConSys->removeConsoleReceiver(s_STDCon);
        delete s_STDCon;
    }

    delete s_Logger;
    delete s_ConSys;
}
