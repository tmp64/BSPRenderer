#ifndef GUI_APP_GUI_APP_BASE_H
#define GUI_APP_GUI_APP_BASE_H
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <appfw/utils.h>
#include <appfw/appfw.h>
#include <appfw/timer.h>
#include <appfw/compiler.h>
#include <appfw/prof.h>
#include <app_base/app_config.h>
#include <gui_app/utils.h>
#include <gui_app/input_system.h>
#include <gui_app/dev_console_dialog.h>
#include <gui_app/profiler_dialog.h>

class GuiAppInfo {
public:
    /**
     * Name of application's directory.
     */
    std::string appDirName = nullptr;

    /**
     * Base app path.
     */
    fs::path baseAppPath;
};

/**
 * A base class for GUI OpenGL apps.
 */
class GuiAppBase : appfw::NoCopy {
public:
    static constexpr float NORMAL_DPI = 96;

    static inline GuiAppBase &getBaseInstance() { return *m_sBaseInstance; }

    GuiAppBase();
    virtual ~GuiAppBase();

    /**
     * Runs the event loop and returns program return code.
     */
    int run();

    /**
     * Called every tick (app_tps_max times per second).
     */
    virtual void tick();

    /**
     * Called every rendering frame (app_fps_max times per second).
     * Swapping is done automatically.
     */
    virtual void draw();

    /**
     * Handles an SDL event.
     * @return true if event was handled, false if ignored.
     */
    virtual bool handleSDLEvent(SDL_Event event);

    /**
     * Called when window size is changed.
     */
    virtual void onWindowSizeChange(int wide, int tall);

    /**
     * Quits the app.
     */
    void quit();

    /**
     * Returns time since app startup in seconds, updated every tick.
     */
    inline double getCurrentTime() { return m_flTime; }

    /**
     * Returns whether developer console is visible.
     */
    bool isConsoleVisible();

    /**
     * Shows/hides the dev console.
     */
    void setConsoleVisible(bool state);

    /**
     * Whether automatically call glClear.
     */
    inline bool isAutoClearEnabled() { return m_bAutoClear; }

    /**
     * Enable/disable autoclear.
     */
    inline void setAutoClearEnabled(bool state) { m_bAutoClear = state; }

    /**
     * Sets autoclear color.
     */
    inline void setAutoClearColor(glm::vec4 color) { m_vAutoClearColor = color; }

    /**
     * Returns base path of the app.
     */
    fs::path getBaseAppPath();

    /**
     * Returns app configuration.
     */
    AppConfig &getConfig();

    /**
     * Returns current window size.
     */
    inline glm::ivec2 getWindowSize() { return m_vWindowSize; }

    /**
     * Shows framerate stats. Must be called inside an ImGui frame.
     */
    void showStatsUI();

    /**
     * Returns the current tickrate (how many times tick() is called)
     */
    double getTickRate();

protected:
    AppFWAppComponent m_AppFWComponent;
    FileSystemAppComponent m_FSComponent;
    AppConfig m_AppConfig;
    AppConfigInitAppComponent m_AppConfigInit;
    OpenGLAppComponent m_OpenGLComponent;
    MainWindowAppComponent m_Window;
    ImGuiAppComponent m_ImGui;
    RendererSystemAppComponent m_RendererSystem;
    InputSystem m_InputSystem;
    DevConsoleDialog m_DevConsole;
    ProfilerDialog m_ProfDialog;

    bool m_bIsRunning = false;
    bool m_bIsConsoleVisible = false;
    bool m_bAutoClear = true;
    glm::vec4 m_vAutoClearColor = glm::vec4(0x0D / 255.0f, 0x27 / 255.0f, 0x40 / 255.0f, 1);
    glm::ivec2 m_vWindowSize = glm::ivec2(0, 0);

    appfw::Timer m_TickTimer;
    appfw::Timer m_DrawTimer;
    unsigned long long m_iTickCount = 0;
    unsigned long long m_iFrameCount = 0;
    double m_flLastTickTime = 0;
    double m_flLastFrameTime = 0;
    double m_flTime = 0;    // Current time in seconds, updated every tick, starts with zero.

    appfw::ProfData m_DrawProfData;
    appfw::ProfData m_TickProfData;

    /**
     * Called every event loop iteration.
     */
    //void internalEventLoopIter();

    /**
     * Called when tick time comes up.
     */
    void internalTick();

    /**
     * Called when drawing time comes up
     */
    void internalDraw();

    static inline GuiAppBase *m_sBaseInstance = nullptr;
};

/**
 * Shows an error message dialog and exits the program.
 */
[[noreturn]] void app_vfatalError(const char *format, fmt::format_args args);

/**
 * Shows an error message dialog and exits the program.
 */
template <typename... Args>
[[noreturn]] inline void app_fatalError(const char *format, const Args &... args) {
    app_vfatalError(format, fmt::make_format_args(args...));
}

/**
 * Returns info for current app's initialization.
 * Must be implemented by the app.
 */
const GuiAppInfo &app_getInitInfo();

/**
 * Creates an instance of GuiAppBase.
 * Must be implemented by the app.
 */
std::unique_ptr<GuiAppBase> app_createSingleton();

/**
 * Calls main of a GUI app. Must be called from main() or its equivalent.
 */
int app_runmain(int argc, char **argv);

#if defined(SDL_MAIN_NEEDED) || defined(SDL_MAIN_AVAILABLE)

// Main is redefined by SDL2.
#if PLATFORM_ANDROID

// On Android apps are built as shared libraries, and SDL_main must be exported.
extern "C" AFW_DLL_EXPORT int main(int argc, char **argv);

#else

/**
 * SDL2 application entry point.
 */
extern "C" int main(int argc, char **argv);

#endif

#endif

#endif
