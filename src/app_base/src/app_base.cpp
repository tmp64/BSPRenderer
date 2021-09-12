#include <iostream>
#include <appfw/timer.h>
#include <appfw/init.h>
#include <app_base/app_base.h>
#include <app_base/app_component.h>

ConVar<double> app_fps_max("app_fps_max", 30, "Application framerate/tickrate");

ConCommand cmd_quit("quit", "Exits the application", []() {
    AFW_ASSERT_REL_MSG(AppBase::isBaseReady(), "Can't quit non-initialized app");
    AppBase::getBaseInstance().quit();
});

AppBase::ComponentSubsystem *AppBase::ComponentSubsystem::m_spInstance = nullptr;
AppBase *AppBase::m_spInstance = nullptr;

AppBase::ComponentSubsystem::ComponentSubsystem() {
    AFW_ASSERT(!m_spInstance);
    m_spInstance = this;
}

AppBase::ComponentSubsystem::~ComponentSubsystem() {
    AFW_ASSERT(m_spInstance == this);
    m_spInstance = nullptr;
}

void AppBase::ComponentSubsystem::finalize() {
    components.shrink_to_fit();
}

void AppBase::ComponentSubsystem::addComponent(AppComponent *comp) {
    AFW_ASSERT_REL(!AppBase::getBaseInstance().isBaseReady() ||
                   !AppBase::getBaseInstance().m_bInitialized);

    components.push_back(comp);
    markTickListAsDirty();
}

void AppBase::ComponentSubsystem::markTickListAsDirty() {
    bTickListIsDirty = true;
}

void AppBase::ComponentSubsystem::updateTickList() {
    if (bTickListIsDirty) {
        bTickListIsDirty = false;
        tickableComponents.clear();

        for (AppComponent *comp : components) {
            if (comp->isTickEnabled()) {
                tickableComponents.push_back(comp);
            }
        }
    }
}

AppBase::AppBase() {
    AFW_ASSERT(!m_spInstance);
    m_spInstance = this;
}

AppBase::~AppBase() {
    AFW_ASSERT(m_spInstance == this);
    m_spInstance = nullptr;
}

int AppBase::run() {
    AFW_ASSERT_REL_MSG(!m_bInitialized, "App cannot be restarted after exit");
    m_bInitialized = true;
    m_Components.finalize();

    // Set up profiler
    m_ProfData.setName("Event Loop");
    m_ProfData.begin();

    // Late init
    for (AppComponent *comp : m_Components.components) {
        comp->lateInit();
    }

    lateInit();

    // Timings
    int64_t oldTime = -(int64_t)((1.0 / app_fps_max.getValue()) *
                                   1'000'000'000); // sane value so tick time is not zero
    appfw::Timer timer;
    m_bIsRunning = true;
    timer.start();

    while (m_bIsRunning) {
        int64_t timeNano = timer.ns();
        int64_t tickTime = timeNano - oldTime;
        oldTime = timeNano;
        m_flTime = (float)(timeNano / 1'000'000'000.0);
        m_flTickTime = (float)(tickTime / 1'000'000'000.0);

        int64_t minTickTime = (int64_t)((1.0 / app_fps_max.getValue()) * 1'000'000'000);

        processTick();

        if (timer.ns() - timeNano < minTickTime) {
            appfw::Prof prof("(frame limit)");

            // Sleep for some time (but leave 8 ms for yielding)
            int64_t tickEndTime = timeNano + minTickTime;
            int64_t sleepEndTime = tickEndTime - 8000000;
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleepEndTime - timer.ns()));

            // Yield loop
            while (timer.ns() < tickEndTime) {
                std::this_thread::yield();
            }
        }
    }

    printi("Application is shutting down. Goodbye!");
    return m_iExitCode;
}

void AppBase::quit(int exitCode) {
    m_bIsRunning = false;
    
    if (exitCode != 0) {
        // Multiple quits will override current exit code only if their code is not zero
        m_iExitCode = exitCode;
    }
}

void AppBase::processTick() {
    try {
        m_ProfData.end();
        m_ProfData.begin();

        appfw::mainLoopTick();

        m_Components.updateTickList();

        {
            appfw::Prof prof("App Begin Tick");
            beginTick();
        }

        // Tick components
        for (AppComponent *comp : m_Components.tickableComponents) {
            comp->tick();
        }

        {
            appfw::Prof prof("App Tick");
            tick();
        }

        // Late tick components
        for (AppComponent *comp : m_Components.tickableComponents) {
            comp->lateTick();
        }

        {
            appfw::Prof prof("App Late Tick");
            lateTick();
        }

        {
            appfw::Prof prof("End Tick");
            endTick();
        }
    } catch (const std::exception &e) {
        app_fatalError("Unhandled exception during tick processing:\n{}", e.what());
    }
}

double AppBase::getFrameRate() {
    return app_fps_max.getValue();
}

static AppFatalErrorHandler s_FatalErrorHandler = nullptr;

[[noreturn]] void app_vfatalError(const char *format, fmt::format_args args) {
    std::string msg = fmt::vformat(format, args);
    std::cerr << "A fatal error has occured.\n" << msg << "\n";

    if (s_FatalErrorHandler) {
        s_FatalErrorHandler(msg);
    }

    std::abort();
}

void app_setFatalErrorHandler(AppFatalErrorHandler handler) {
    s_FatalErrorHandler = handler;
}
