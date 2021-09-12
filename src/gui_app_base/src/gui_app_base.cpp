#include <appfw/appfw.h>
#include <gui_app_base/gui_app_base.h>
#include "dev_console_dialog.h"
#include "dev_console_overlay.h"
#include "profiler_dialog.h"

ConVar<bool> app_vsync("app_vsync", true, "Enable vertical synchronization");
ConVar<bool> app_vsync_finish("app_vsync_finish", true,
                              "Wait for image to be fully displayed before rendering");

GuiAppBase::GuiAppBase() {
    m_pDevConsole = std::make_unique<DevConsoleDialog>();
    appfw::getConsole().addConsoleReceiver(m_pDevConsole.get());

    m_pDevConsoleOverlay = std::make_unique<DevConsoleOverlay>();
    appfw::getConsole().addConsoleReceiver(m_pDevConsoleOverlay.get());

    m_pProfilerUI = std::make_unique<ProfilerDialog>();

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
}

GuiAppBase::~GuiAppBase() {
    appfw::getConsole().removeConsoleReceiver(m_pDevConsoleOverlay.get());
    appfw::getConsole().removeConsoleReceiver(m_pDevConsole.get());
}

void GuiAppBase::lateInit() {
    // Update window size
    int wide, tall;
    SDL_GetWindowSize(m_MainWindow.getWindow(), &wide, &tall);
    onWindowSizeChange(wide, tall);
}

void GuiAppBase::beginTick() {
    pollEvents();
    m_ImGui.beginTick();
}

void GuiAppBase::tick() {
    m_pDevConsole->Draw("Developer Console", nullptr);
    m_pDevConsoleOverlay->tick();
    m_pProfilerUI->tick();
}

void GuiAppBase::endTick() {
    m_ImGui.endTick();
    refreshScreen();
}

void GuiAppBase::refreshScreen() {
    if (app_vsync_finish.getValue()) {
        // https://www.khronos.org/opengl/wiki/Swap_Interval
        // GPU vs CPU synchronization
        // This should remove lag caused by VSync
        appfw::Prof prof("glFinish");
        glFinish();
    }

    drawBackground();
    drawForeground();

    {
        appfw::Prof prof("Swap Buffers");
        SDL_GL_SwapWindow(m_MainWindow.getWindow());
    }
}

void GuiAppBase::drawBackground() {
    if (m_bAutoClear) {
        glClearColor(m_vAutoClearColor.r, m_vAutoClearColor.g, m_vAutoClearColor.b,
                     m_vAutoClearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
}

void GuiAppBase::drawForeground() {
    m_ImGui.render();
}

bool GuiAppBase::handleSDLEvent(const SDL_Event &event) {
    if (InputSystem::get().handleSDLEvent(event)) {
        return true;
    }

    switch (event.type) {
    case SDL_WINDOWEVENT: {
        if (event.window.windowID == SDL_GetWindowID(m_MainWindow.getWindow())) {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED: {
                onWindowSizeChange(event.window.data1, event.window.data2);
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

void GuiAppBase::onWindowSizeChange(int wide, int tall) {
    m_vWindowSize.x = wide;
    m_vWindowSize.y = tall;
}

void GuiAppBase::pollEvents() {
    appfw::Prof prof("Poll Events");
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        handleSDLEvent(event);
    }
}
