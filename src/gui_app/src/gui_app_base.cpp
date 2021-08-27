#include <algorithm>
#include <iostream>
#include <appfw/init.h>
#include <gui_app/gui_app_base.h>

ConVar<double> app_tps_max("app_tps_max", 100, "Application tickrate");
ConVar<double> app_fps_max("app_fps_max", 60, "Application framerate");
ConVar<bool> app_sync_rates("app_sync_rates", false, "Syncs TPS and FPS, uses value from app_fps_max for the rate.");
ConVar<bool> app_vsync("app_vsync", true, "Enable vertical synchronization");
ConVar<bool> app_vsync_finish("app_vsync_finish", true,
                              "Wait for image to be fully displayed before rendering");

static ConCommand cmd_toggleconsole("toggleconsole", "Toggles console dialog", [](const CmdString &) {
    GuiAppBase::getBaseInstance().setConsoleVisible(!GuiAppBase::getBaseInstance().isConsoleVisible());
});

static ConCommand cmd_quit("quit", "Quits the app", [](const CmdString &) {
    GuiAppBase::getBaseInstance().quit();
});

GuiAppBase::GuiAppBase()
    : m_AppConfig(
          getFileSystem().findExistingFile("base:" +  app_getInitInfo().appDirName + "/app_config.yaml"))
    , m_AppConfigInit(m_AppConfig)
    , m_Window(m_AppConfig)
    , m_ImGui(m_Window) {
    AFW_ASSERT(!m_sBaseInstance);
    m_sBaseInstance = this;

    // TODO: Enable extcon
    //appfw::init::setExtconPort(getConfig().getItem("appfw").get<int>("extcon_port"));

    // Set VSync cvar callback
    app_vsync.setCallback([](const bool &, const bool &newVal) {
        if (newVal) {
            // Try adaptive sync
            if (SDL_GL_SetSwapInterval(-1) == 0) {
                printi("Enabled Adaptive VSync");
            } else {
                // Try VSync
                if (SDL_GL_SetSwapInterval(1) == 0) {
                    printi("Enabled VSync");
                } else {
                    printe("Failed to enable VSync");
                }
            }
        } else {
            // Try to disable VSync
            if (SDL_GL_SetSwapInterval(0) == 0) {
                printi("Disabled VSync");
            } else {
                printe("Failed to disable VSync");
            }
        }

        return true;
    });

    app_vsync.setValue(app_vsync.getValue());

    // ImGUI scale
    float guiScale = getConfig().getItem("gui").get<float>("imgui_scale", 0.0f);

    if (guiScale == 0) {
        guiScale = 1.0f;
        float dpi = 0;

        if (SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr) == 0) {
            if (dpi > NORMAL_DPI) {
                guiScale = dpi / NORMAL_DPI;

                if (appfw::isAndroid()) {
                    guiScale *= 3.f / 5.f;
                }

                printd("Display DPI: {}, UI scale: {}", dpi, guiScale);
            } else if (dpi < NORMAL_DPI) {
                printw("DPI is lower than normal DPI ({} < {}), UI not scaled", dpi, NORMAL_DPI);
            }
        } else {
            printw("Failed to get display DPI: {}", SDL_GetError());
        }
    }

    std::string fontPath = getConfig().getItem("gui").get<std::string>("imgui_font_path", "");
    float fontSize = getConfig().getItem("gui").get<float>("imgui_font_size", 13.0f);

    if (fontPath.empty() || !m_ImGui.loadFont(fontPath, fontSize * guiScale)) {
        printe("ImGui: Using default font");
        ImGui::GetIO().Fonts->AddFontDefault();
    }

    ImGui::GetStyle().ScaleAllSizes(guiScale);

    // Dev console
    appfw::getConsole().addConsoleReceiver(&m_DevConsole);
    InputSystem::get().bindKey(SDL_SCANCODE_GRAVE, "toggleconsole");

    printi("GuiAppBase initialized.");
}

GuiAppBase::~GuiAppBase() {
    AFW_ASSERT(m_sBaseInstance);

    appfw::getConsole().removeConsoleReceiver(&m_DevConsole);

    m_sBaseInstance = nullptr;
    printi("Shutting down GuiAppBase.");
}

int GuiAppBase::run() {
    m_bIsRunning = true;
    m_TickTimer.start();
    m_DrawTimer.start();

    // Update window size
    int wide, tall;
    SDL_GetWindowSize(m_Window.getWindow(), &wide, &tall);
    onWindowSizeChange(wide, tall);

    // Execute pending commands
    appfw::getConsole().processAllCommands();
    appfw::getConsole().processAllCommands();
    appfw::getConsole().processAllCommands();

    // Set up profiler
    m_TickProfData.setName("Ticking");
    m_DrawProfData.setName("Screen redraw");

    appfw::Timer waitTimer;

    while (m_bIsRunning) {
        double targetTicktime = 1 / app_tps_max.getValue();
        double targetFrametime = 1 / app_fps_max.getValue();
        bool syncRates = app_sync_rates.getValue();
        long long timeToWait = 0;

        if (syncRates) {
            m_flLastTickTime = m_TickTimer.dseconds();
            m_TickTimer.start();
            internalTick();

            m_flLastFrameTime = m_flLastTickTime;
            internalDraw();

            m_flTime += m_flLastTickTime;

            double timeLeft = targetFrametime - m_TickTimer.dseconds();
            timeToWait = (long long)(timeLeft * 1000000);
        } else {
            if (m_TickTimer.dseconds() >= targetTicktime) {
                m_flLastTickTime = m_TickTimer.dseconds();
                m_TickTimer.start();
                internalTick();
                m_flTime += m_flLastTickTime;
            }

            if (m_DrawTimer.dseconds() >= targetFrametime) {
                m_flLastFrameTime = m_DrawTimer.dseconds();
                m_DrawTimer.start();
                internalDraw();
            }

            double tickTimeLeft = targetTicktime - m_TickTimer.dseconds();
            double frameTimeLeft = targetFrametime - m_DrawTimer.dseconds();

            timeToWait = (long long)(std::min(tickTimeLeft, frameTimeLeft) * 1000000);
        }

        waitTimer.start();

        while (waitTimer.us() < timeToWait - 4000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        while (waitTimer.us() < timeToWait) {
            std::this_thread::yield();
        }
    }

    return 0;
}

void GuiAppBase::tick() {}

void GuiAppBase::draw() {}

bool GuiAppBase::handleSDLEvent(SDL_Event event) {
    if (InputSystem::get().handleSDLEvent(event)) {
        return true;
    }

    switch (event.type) {
    case SDL_WINDOWEVENT: {
        if (event.window.windowID == SDL_GetWindowID(m_Window.getWindow())) {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
                int wide, tall;
                SDL_GetWindowSize(m_Window.getWindow(), &wide, &tall);
                onWindowSizeChange(wide, tall);
                return true;
            }
            case SDL_WINDOWEVENT_CLOSE: {
                quit();
                return true;
            }
            }
        }

        return false;
    }
    case SDL_QUIT: {
        quit();
        return true;
    }
    }

    return false;
}

fs::path GuiAppBase::getBaseAppPath() { return m_FSComponent.m_BaseAppPath; }

AppConfig &GuiAppBase::getConfig() { return m_AppConfig; }

void GuiAppBase::showStatsUI() {
    ImColor cyan = ImColor(0, 255, 255, 255);
    ImGui::TextColored(cyan, "FPS: %4.0f (%.3f ms)", 1.f / m_flLastFrameTime, m_flLastFrameTime * 1000.f);

    if (!app_sync_rates.getValue()) {
        ImGui::TextColored(cyan, "TPS: %4.0f (%.3f ms)", 1.f / m_flLastTickTime,
                           m_flLastTickTime * 1000.f);
    }
}

double GuiAppBase::getTickRate() {
    return app_sync_rates.getValue() ? app_fps_max.getValue() : app_tps_max.getValue();
}

void GuiAppBase::onWindowSizeChange(int wide, int tall) {
    m_vWindowSize.x = wide;
    m_vWindowSize.y = tall;
}

void GuiAppBase::quit() {
    printi("Quitting the app");
    m_bIsRunning = false;
}

bool GuiAppBase::isConsoleVisible() { return m_bIsConsoleVisible; }

void GuiAppBase::setConsoleVisible(bool state) {
    if (state) {
        InputSystem::get().setGrabInput(false);
        m_bIsConsoleVisible = true;
    } else {
        m_bIsConsoleVisible = false;
    }
}

void GuiAppBase::internalTick() {
    try {
        m_TickProfData.begin();

        {
            appfw::Prof prof("Poll Events");
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                handleSDLEvent(event);
            }
        }

        m_AppFWComponent.tick();
        m_InputSystem.tick();
        m_ImGui.newTick();
        m_RendererSystem.tick();

        if (!InputSystem::get().isInputGrabbed() && m_bIsConsoleVisible) {
            m_DevConsole.Draw("Developer Console", &m_bIsConsoleVisible);
        }

        m_ProfDialog.tick();

        tick();

        ImGui::EndFrame();
        m_TickProfData.end();
    } catch (const std::exception &e) {
        app_fatalError("Unhandled exception in GuiAppBase::internalTick:\n{}", e.what());
    }
}

void GuiAppBase::internalDraw() { 
    try {
        m_DrawProfData.begin();

        if (app_vsync_finish.getValue()) {
            // https://www.khronos.org/opengl/wiki/Swap_Interval
            // GPU vs CPU synchronization
            // This should remove lag caused by VSync
            appfw::Prof prof("glFinish");
            glFinish();
        }

        if (m_bAutoClear) {
            glClearColor(m_vAutoClearColor.r, m_vAutoClearColor.g, m_vAutoClearColor.b, m_vAutoClearColor.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        draw();

        m_ImGui.draw();

        {
            appfw::Prof prof("Swap Buffers");
            SDL_GL_SwapWindow(m_Window.getWindow());
        }

        m_DrawProfData.end();
    } catch (const std::exception &e) {
        app_fatalError("Unhandled exception in GuiAppBase::internalDraw:\n{}", e.what());
    }
}

void app_vfatalError(const char *format, fmt::format_args args) {
    std::string msg = fmt::vformat(format, args);

    if (SDLAppComponent::isInitialized()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", msg.c_str(), nullptr);

        if (appfw::isInitialized()) {
            printfatal("Fatal Error: {}\n", msg);
        }
    }

    std::cerr << "Fatal Error: " << msg << "\n";
    std::abort();
}
