#include <iostream>

#include <appfw/console/console_system.h>
#include <appfw/console/std_console.h>
#include <appfw/console/con_item.h>
#include <appfw/dbg.h>
#include <appfw/init.h>
#include <appfw/services.h>
#include <appfw/command_line.h>

static bool s_IsRunning = false;

ConVar<std::string> test_cvar("test_cvar", "test", "A test console variable.");
ConVar<int> test_cvar_int("test_cvar_int", 1, "A test integer console variable.");
ConVar<float> test_cvar_float("test_cvar_float", 3.1415926535f, "A test integer console variable.");
ConVar<bool> test_cvar_bool("test_cvar_bool", true, "sdfsdfsdf");

//ConVar<bool> run("run", true, "Enables/disables the app");

ConCommand quit_cmd("quit", "Exits the app", [](auto &) { s_IsRunning = false; });

int realMain(int, char **) {
    appfw::init::init();

    logWarn("Test");

    try {
        // --arg1 "val1 with spaces" --fl1 -fA +cmd cmdarg
        int argc = 7;
        char *argv[] = {"exec_name", "--arg1", "val1 with spaces", "--fl1", "-fA", "+cmd", "cmdarg"};
        appfw::CommandLine cmd;
        cmd.parseCommandLine(argc, argv);
        logInfo("parsed");
    }
    catch (const std::exception &e) {
        logError("{}", e.what());
    }
    

    /*s_IsRunning = true;

    while (s_IsRunning) {
        appfw::init::mainLoopTick();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
    }*/

    logDebug("Quitting...");
    appfw::init::shutdown();
    return 0;
}

int main(int argc, char **argv) { return realMain(argc, argv); }
