#include <iostream>

#include <appfw/console/console_system.h>
#include <appfw/console/std_console.h>
#include <appfw/console/con_item.h>
#include <appfw/dbg.h>
#include <appfw/init.h>
#include <appfw/services.h>

#include <bsp/level.h>
#include <bsp/wad_file.h>
#include <renderer/base_renderer.h>

static bool s_IsRunning = false;

ConVar<std::string> test_cvar("test_cvar", "test", "A test console variable.");
ConVar<int> test_cvar_int("test_cvar_int", 1, "A test integer console variable.");
ConVar<float> test_cvar_float("test_cvar_float", 3.1415926535f, "A test integer console variable.");
ConVar<bool> test_cvar_bool("test_cvar_bool", true, "sdfsdfsdf");

//ConVar<bool> run("run", true, "Enables/disables the app");

ConCommand quit_cmd("quit", "Exits the app", [](auto &) { s_IsRunning = false; });

class TestRenderer : public BaseRenderer {
public:
    virtual void createSurfaces() override {}
    virtual void destroySurfaces() override {}
    virtual void drawWorldSurfaces(const std::vector<size_t> &surfaceIdxs) noexcept override {
        int count = 0;
        for (size_t idx : surfaceIdxs) {
            count++;
            logWarn("Surface {}", count);
            LevelSurface &surf = m_BaseSurfaces[idx];

            for (glm::vec3 v : surf.vertices) {
                logInfo("    {} {} {}", v.x, v.y, v.z);
            }
            logInfo("");
        }
        logInfo("Need to draw {} surfaces", surfaceIdxs.size());
    }
};

int realMain(int, char **) {
    appfw::init::init();

    logWarn("Test");

    /*bsp::Level lvl("maps/test1.bsp");
    TestRenderer renderer;
    renderer.setLevel(&lvl);
    renderer.draw(BaseRenderer::DrawOptions{{0.f, 0.f, 0.f}});*/

    bsp::WADFile wad("halflife.wad");

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
