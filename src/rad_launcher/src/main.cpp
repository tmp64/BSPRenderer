#include <nlohmann/json.hpp>
#include <appfw/timer.h>
#include <appfw/init.h>
#include <appfw/appfw.h>
#include <appfw/command_line.h>
#include <app_base/app_config.h>
#include <rad/rad_sim.h>

AppConfig g_AppConfig;
bsp::Level g_Level;
std::string g_LevelPath;

void initApp();
void loadLevel();

bool g_bWasProgressCalled = false;

void finishProgress() {
    if (g_bWasProgressCalled) {
        g_bWasProgressCalled = false;
        printf("\n");
    }
}

void progressCallback(double progress) {
    static double lastProgress = 0;

    if (progress == 0) {
        if (lastProgress != 0) {
            lastProgress = 0;
            printf("0%%..");
            g_bWasProgressCalled = true;
        }
    } else {
        if (std::floor(progress * 100) != lastProgress) {
            lastProgress = std::floor(progress * 100);
            printf("%.0f%%..", lastProgress);
            g_bWasProgressCalled = true;
        }

        if (progress == 1) {
            finishProgress();
        }
    }
}

int main(int argc, char **argv) {
    int returnCode = 0;
    appfw::InitComponent appfwInit(appfw::InitOptions().setArgs(argc, argv));

    printn("---------------------------------------------------");
    printn("Radiosity simulator for Half-Life BSP Renderer");
    printn("https://github.com/tmp64/BSPRenderer");
    printn("---------------------------------------------------");

    try {
        initApp();

        appfw::Timer timer;
        timer.start();

        rad::RadSim rad;
        rad.setAppConfig(&g_AppConfig);
        rad.setProgressCallback(progressCallback);
        
        loadLevel();
        rad.setLevel(&g_Level, g_LevelPath);
        rad.loadLevelConfig();

        if (getCommandLine().doesArgHaveValue("--bounce")) {
            rad.setBounceCount(getCommandLine().getArgInt("--bounce"));
        }

        bool bCanReuseFiles = !getCommandLine().isFlagSet("--no-reuse");

        printi("Base patch size: {}", rad.getPatchSize());
        printi("Bounce count: {}", rad.getBounceCount());

        if (bCanReuseFiles) {
            printi("Loading vismat...");
            if (!rad.loadVisMat()) {
                rad.calcVisMat();
            }
        } else {
            rad.calcVisMat();
        }

        rad.calcViewFactors();

        rad.bounceLight();
        rad.writeLightmaps();

        timer.stop();
        printi("Done! Time taken: {:.3} s.", timer.dseconds());
    }
    catch (const std::exception &e) {
        printe("{}", e.what());
        returnCode = 1;
    }

    return returnCode;
}

void initApp() {
    // Init file system
    fs::path baseAppPath = fs::current_path();
    printi("Base app path: {}", baseAppPath.u8string());
    getFileSystem().addSearchPath(baseAppPath, "base");

    // Load app config
    g_AppConfig.loadJsonFile(getFileSystem().findExistingFile("base:bspviewer/app_config.json"));
    g_AppConfig.mountFilesystem();
}

void loadLevel() {
    std::string map = getCommandLine().getArgString("--map");
    g_LevelPath = fmt::format("assets:maps/{}.bsp", map);
    printi("Loading level {}", g_LevelPath);
    g_Level.loadFromFile(getFileSystem().findExistingFile(g_LevelPath));
}