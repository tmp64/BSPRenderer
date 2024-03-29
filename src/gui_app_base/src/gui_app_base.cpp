#include <stb_image_write.h>
#include <appfw/appfw.h>
#include <gui_app_base/gui_app_base.h>
#include "dev_console_dialog.h"
#include "profiler_dialog.h"

ConVar<bool> app_vsync("app_vsync", true, "Enable vertical synchronization");
ConVar<bool> app_vsync_finish("app_vsync_finish", true,
                              "Wait for image to be fully displayed before rendering");
ConCommand cmd_snapshot("snapshot", "Take a snapshot of the whole screen");

KeyBind key_snapshot("Take a snapshot", KeyCode::F5);

GuiAppBase::GuiAppBase()
    : m_Config(CONFIG_FILE_PATH) {
    m_pDevConsole = std::make_unique<DevConsoleDialog>();
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

    cmd_snapshot.setCallback([&]() { takeSnapshot(); });
}

GuiAppBase::~GuiAppBase() = default;

void GuiAppBase::takeSnapshot() {
    m_bPendingSnapshot = true;
}

void GuiAppBase::lateInit() {
    // Update window size
    int wide, tall;
    SDL_GetWindowSize(m_MainWindow.getWindow(), &wide, &tall);
    onWindowSizeChange(wide, tall);
    AFW_ASSERT(wide != 0 && tall != 0);
}

void GuiAppBase::beginTick() {
    m_InputSystem.beginTick();
    pollEvents();
    m_ImGui.beginTick();
}

void GuiAppBase::tick() {
    if (AppExtconComponent::get().isHostFocusRequested()) {
        SDL_Window *mainWindow = MainWindowComponent::get().getWindow();
        SDL_RaiseWindow(mainWindow);
    }

    m_pDevConsole->tick();
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

        if (m_bPendingSnapshot || key_snapshot.isPressed()) {
            saveSnapshot();
        }

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
    InputSystem::get().handleSDLEvent(event);

    switch (event.type) {
    case SDL_WINDOWEVENT: {
        if (event.window.windowID == SDL_GetWindowID(m_MainWindow.getWindow())) {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_MOVED: {
                m_MainWindow.saveWindowPos();
                return true;
            }
            case SDL_WINDOWEVENT_RESIZED: {
                m_MainWindow.saveWindowSize();
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

void GuiAppBase::saveSnapshot() {
    m_bPendingSnapshot = false;

    // Find a suitable file
    int fileIdx = 0;
    fs::path path;

    for (; fileIdx < 10000; fileIdx++) {
        path = getFileSystem().getFilePath(fmt::format("assets:snapshots/{:04d}.png", fileIdx));

        if (fs::exists(path)) {
            path.clear();
        } else {
            break;
        }
    }

    if (path.empty()) {
        printe("Snapshot: file counter exhausted");
        return;
    }

    // Read framebuffer
    glm::ivec2 fbsize = m_vWindowSize;
    size_t dataSize = size_t(4) * fbsize.x * fbsize.y;
    std::vector<uint8_t> pixels(dataSize);
    glReadPixels(0, 0, fbsize.x, fbsize.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Save into a file
    std::ofstream file(path, std::ios::binary);

    if (file.fail()) {
        const char *error = strerror(errno);
        printe("Snapshot: failed to open file '{}': '{}'", path.u8string(), error);
        return;
    }

    stbi_flip_vertically_on_write(true);
    stbi_write_png_to_func(
        [](void *context, void *data, int size) {
            std::ofstream &file = *static_cast<std::ofstream *>(context);
            file.write(reinterpret_cast<const char *>(data), size);
        },
        &file, fbsize.x, fbsize.y, 4, pixels.data(), 4 * fbsize.x);

    if (file.fail()) {
        const char *error = strerror(errno);
        printe("Snapshot: failed to write into file '{}': '{}'", path.u8string(), error);
        return;
    }

    printi("Snapshot saved to {}", path.u8string());
}
