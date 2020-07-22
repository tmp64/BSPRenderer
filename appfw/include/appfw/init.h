#ifndef APPFW_INIT_H
#define APPFW_INIT_H
#include <appfw/platform.h>

namespace appfw {

namespace init {

struct InitOptions {
    /**
     * Whether or not console system must be enabled.
     * Default: on
     */
    bool bEnableConsole = true;

    /**
     * Register all available console items on init.
     * Requires: console
     * Default: on
     */
    bool bRegisterItems = true;

    /**
     * Enable stdio console.
     * Requires: console
     * Default: on
     */
    bool bEnableSTDConsole = true;

    /**
     * Whether or nor logger must be enabled.
     * Requires: console
     * Default: on
     */
    bool bEnableLogger = true;

    /**
     * Print appfw config during init.
     * Requires: logger
     * Default: on in debug build
     */
    bool bPrintConfig = platform::isDebugBuild();

    /**
     * Print a warning if main loop tick takes too long.
     * Default: on
     */
    bool bLoopStuckCheck = true;

    /**
     * Time (in milliseconds) for a loop stick warning.
     * Default: 2000 ms
     */
    int iLoopStickTime = 2000;
};

/**
 * Initializes appfw with specified options.
 * @see InitOptions
 */
void init(InitOptions options = InitOptions());

/**
 * Does internal processing. Must be called in the main event loop.
 */
void mainLoopTick();

/**
 * Shutdowns appfw. Must be called when app is shutting down.
 */
void shutdown();

} // namespace init

} // namespace appfw

#endif
