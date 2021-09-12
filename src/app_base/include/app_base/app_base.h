#ifndef APP_BASE_H
#define APP_BASE_H
#include <vector>
#include <fmt/format.h>
#include <appfw/utils.h>
#include <appfw/prof.h>
#include <app_base/components.h>

class AppComponent;

//! Contains app-specific initialization info
struct AppInitInfo {
    //! Name of application's directory (that contains app_config.yaml)
    std::string appDirName = nullptr;

    //! Base app path - path with application directories
    //! Can be empty (will use workdir instead)
    fs::path baseAppPath;
};

//! A base class for an appfw application with event loop.
//! appfw must be initialized prior to construction of this object.
class AppBase : appfw::NoMove {
public:
    //! Returns the instance of AppBase.
    static inline AppBase &getBaseInstance() { return *m_spInstance; }

    //! Returns whether AppBase is constructed.
    static inline bool isBaseReady() { return m_spInstance != nullptr; }

    AppBase();
    virtual ~AppBase();

    //! Runs the event loop, blocks until the program finishes.
    //! @returns program return code
    int run();

    //! Instructs the app to shutdown gracefully.
    //! @param  exitCode    The program exit (return) code
    void quit(int exitCode = 0);

    //! Runs a full app tick (this calls tick methods below).
    void processTick();

    //! Returns the time since app start up
    inline float getTime() { return m_flTime; }

    //! Returns the time since last tick
    inline float getTimeDelta() { return m_flTickTime; }

    //! Returns the app config
    inline AppConfig &getConfig() { return m_Config.getConfig(); }

    //! Returns the app framerate.
    double getFrameRate();

protected:
    //! Called after all components lateInit
    virtual void lateInit() {}

    //! Called before any components tick
    virtual void beginTick() {}

    //! Called after all components tick
    virtual void tick() {}

    //! Called after all components lateTick
    virtual void lateTick() {}

    //! Called at the end of processTick
    virtual void endTick() {}

private:
    struct ComponentSubsystem : appfw::NoMove {
        std::vector<AppComponent *> components;
        std::vector<AppComponent *> tickableComponents;
        bool bTickListIsDirty = true;

        ComponentSubsystem();
        ~ComponentSubsystem();
        void finalize();
        void addComponent(AppComponent *comp);
        void markTickListAsDirty();
        void updateTickList();

        static ComponentSubsystem *m_spInstance;
    };

    bool m_bInitialized = false; //!< If true, app is fully initialized
    bool m_bIsRunning = false;   //!< Whether the app is running
    int m_iExitCode = 0;         //!< App exit code
    float m_flTime = 0;          //!< Time since app start
    float m_flTickTime = 0;      //!< Time since the beginning of last tick
    appfw::ProfData m_ProfData;

    ComponentSubsystem m_Components;
    AppFilesystemComponent m_FileSystem;
    AppConfigComponent m_Config;

    static AppBase *m_spInstance;
    friend class AppComponent;
};

// Function that will be called on fatal error (e.g. to show a dialog)
using AppFatalErrorHandler = void (*)(const std::string &msg) noexcept;

//! Returns info for current app's init info.
//! Must be implemented by the app.
const AppInitInfo &app_getInitInfo();

//! Creates the instance of GuiAppBase.
//! Must be implemented by the app.
std::unique_ptr<AppBase> app_createSingleton();

//! Shows an error message and exits the program.
[[noreturn]] void app_vfatalError(const char *format, fmt::format_args args);

//! Shows an error message and exits the program.
template <typename... Args>
[[noreturn]] inline void app_fatalError(const char *format, const Args &...args) {
    app_vfatalError(format, fmt::make_format_args(args...));
}

//! Sets the fatal error handler.
void app_setFatalErrorHandler(AppFatalErrorHandler handler);

#endif
