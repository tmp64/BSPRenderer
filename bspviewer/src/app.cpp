#include <ctime>
#include <iostream>
#include <fstream>
#include <appfw/init.h>
#include <appfw/services.h>
#include <appfw/compiler.h>
#include <glad/glad.h>
#include <renderer/shader_manager.h>
#include <renderer/material_manager.h>
#include <renderer/frame_console.h>
#include <renderer/textured_renderer.h>

#include "app.h"
#include "demo.h"

static ConCommand quit_cmd("quit", "Exits the app", [](auto &) { App::get().quit(); });

static ConCommand cmd_map("map", "Loads a map", [](const appfw::ParsedCommand &cmd) {
    if (cmd.size() != 2) {
        logInfo("Usage: map <map name>");
        return;
    }

    App::get().loadMap(cmd[1]);
});

static ConCommand cmd_dem_stop("dem_stop", "Stops demo playing/recording", [](const appfw::ParsedCommand &) {
    App::get().stopDemo();
});

static ConCommand cmd_dem_record("dem_record", "Records a demo", [](const appfw::ParsedCommand &cmd) {
    if (cmd.size() != 2) {
        logInfo("Usage: dem_record <demo name>");
        return;
    }

    App::get().recordDemo(cmd[1]);
});

static ConCommand cmd_dem_play("dem_play", "Plays a demo", [](const appfw::ParsedCommand &cmd) {
    if (cmd.size() != 2) {
        logInfo("Usage: dem_play <demo name>");
        return;
    }

    App::get().playDemo(cmd[1]);
});

static ConVar<float> dem_tickrate("dem_tickrate", 60.f, "Demo tickrate");

static ConVar<float> fps_max("fps_max", 100, "Maximum FPS",
                             [](const float &, const float &newVal) { return newVal >= 10.f; });

static ConVar<float> m_sens("m_sens", 0.15f, "Mouse sensitivity (degrees/pixel)");
static ConVar<float> cam_speed("cam_speed", 1000.f, "Camera speed");
static ConVar<float> fov("fov", 120.f, "Horizontal field of view");

//----------------------------------------------------------------

static const char *getGlErrorString(GLenum errorCode) {
    switch (errorCode) {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "< Unknown >";
    }
}

static void gladPostCallback(const char *name, void *, int, ...) {
    GLenum errorCode = glad_glGetError();

    if (errorCode != GL_NO_ERROR) {
        logError("OpenGL Error: {} (0x{:X}) in {}", getGlErrorString(errorCode), (int)errorCode, name);
        AFW_DEBUG_BREAK();
    }
}

//----------------------------------------------------------------

App::App() {
    AFW_ASSERT(!m_sSingleton);
    m_sSingleton = this;

    SDL_Init(SDL_INIT_VIDEO);
    appfw::init::init();

    // Load OpenGL
    if (SDL_GL_LoadLibrary(nullptr)) {
        fatalError("Failed to load OpenGL library: "s + SDL_GetError());
    }

    if (appfw::platform::isDebugBuild()) {
        logDebug("Using debug GL context.");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }

    // Create window
    m_pWindow = SDL_CreateWindow("BSP Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900,
                                 SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!m_pWindow) {
        fatalError("Failed to create window: "s + SDL_GetError());
    }

    // Create OpenGL context
    m_GLContext = SDL_GL_CreateContext(m_pWindow);

    if (!m_GLContext) {
        fatalError("Failed to create window: "s + SDL_GetError());
    }

    // Init OpenGL
    if (!gladLoadGLLoader([](const char *name) { return SDL_GL_GetProcAddress(name); })) {
        fatalError("Failed load OpenGL functions.");
    }

    glad_set_post_callback(gladPostCallback);

    logInfo("OpenGL Version {}.{} loaded", GLVersion.major, GLVersion.minor);

    if (!GLAD_GL_VERSION_3_3) {
        fatalError(fmt::format("At least OpenGL 3.3 is required. Available {}.{}", GLVersion.major, GLVersion.minor));
    }

    // Init renderer
    ShaderManager::get().init();

    MaterialManager::get().init();
    MaterialManager::get().addWadFile("halflife.wad");

    m_pFrameConsole = new FrameConsole();
    m_pRenderer = new TexturedRenderer();
    loadMap("crossfire");

    updateViewportSize();
    setMouseInputEnabled(false);
}

App::~App() {
    AFW_ASSERT(m_sSingleton);

    // Shutdown renderer
    delete m_pRenderer;
    delete m_pFrameConsole;
    MaterialManager::get().shutdown();
    ShaderManager::get().shutdown();

    // Remove GL context
    SDL_GL_DeleteContext(m_GLContext);
    m_GLContext = nullptr;

    // Destroy window
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;

    appfw::init::shutdown();

    SDL_Quit();

    m_sSingleton = nullptr;
}

//----------------------------------------------------------------

int App::run() {
    m_bIsRunning = true;

    while (m_bIsRunning) {
        m_Timer.start();

        try {
            tick();
        } catch (const std::exception &e) { fatalError("Exception in tick(): "s + e.what()); }

        // Delay
        long long minFrameTime = (unsigned)(1000000 / fps_max.getValue());
        while (m_Timer.elapsedMicroseconds() < minFrameTime - 4000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        while (m_Timer.elapsedMicroseconds() < minFrameTime) {
            std::this_thread::yield();
        }

        m_iLastFrameMicros = (unsigned)m_Timer.elapsedMicroseconds();
        m_flLastFrameTime = m_iLastFrameMicros / 1000000.f;
        m_iTimeMicros += m_iLastFrameMicros;

        m_StatsWriter.add();
    }

    return m_iReturnCode;
}

//----------------------------------------------------------------

void App::tick() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        handleSDLEvent(event);
    }

    checkKeys();

    if (m_DemoState == DemoState::Record) {
        if (m_DemoWriter.hasTimeCome(m_iTimeMicros)) {
            DemoTick tick;

            tick.vViewOrigin = m_Pos;
            tick.vViewAngles = m_Rot;
            tick.flFov = fov.getValue();

            m_DemoWriter.addFrame(tick);
        }
    } else if (m_DemoState == DemoState::Play) {
        if (m_DemoReader.hasTimeCome(m_iTimeMicros)) {
            const DemoTick &tick = m_DemoReader.getTick();

            m_Pos = tick.vViewOrigin;
            m_Rot = tick.vViewAngles;
            fov.setValue(tick.flFov);

            m_DemoReader.advance();

            if (m_DemoReader.isEnd()) {
                stopDemo();
            }
        }
    }

    draw();

    appfw::init::mainLoopTick();
}

void App::quit(int code) {
	m_iReturnCode = code;
    m_bIsRunning = false;
}

void App::handleSDLEvent(SDL_Event event) {
    switch (event.type) {
    case SDL_WINDOWEVENT: {
        if (event.window.windowID == SDL_GetWindowID(m_pWindow)) {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
                updateViewportSize();
                break;
            }
            case SDL_WINDOWEVENT_CLOSE: {
                quit();
                break;
            }
            }

        }

        break;
    }
    case SDL_KEYDOWN: {
        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            setMouseInputEnabled(!isMouseInputEnabled());
        } else if (event.key.keysym.scancode == SDL_SCANCODE_F3) {
            m_bDrawDebugText = !m_bDrawDebugText;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_F8) {
            stopDemo();
        }
        break;
    }
    case SDL_MOUSEMOTION: {
        if (isMouseInputEnabled() && m_DemoState != DemoState::Play) {
            glm::vec3 rot = m_Rot;

            rot.y -= event.motion.xrel * m_sens.getValue();
            rot.x += event.motion.yrel * m_sens.getValue();

            if (rot.x < -90.0f) {
                rot.x = -90.0f;
            } else if (rot.x > 90.0f) {
                rot.x = 90.0f;
            }

            if (rot.y < 0.0f) {
                rot.y = 360.0f + rot.y;
            } else if (rot.y > 360.f) {
                rot.y = rot.y - 360.0f;
            }
            // sf::Mouse::setPosition({xt, yt});
            m_Rot = rot;
        }
        break;
    }
    case SDL_QUIT: {
        quit();
        break;
    }
    }
}

void App::draw() {
    glClearColor(0, 0.5f, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_pFrameConsole->reset();
    
    BaseRenderer::DrawOptions options;
    options.viewOrigin = m_Pos;
    options.viewAngles = m_Rot;
    options.fov = fov.getValue();
    options.aspect = m_flAspectRatio;
    m_LastDrawStats = m_pRenderer->draw(options);

    if (m_bDrawDebugText) {
        drawDebugText(m_LastDrawStats);
    }

    SDL_GL_SwapWindow(m_pWindow);
}

void App::drawDebugText(const BaseRenderer::DrawStats &stats) {
    m_pFrameConsole->printLeft(FrameConsole::Cyan, fmt::format("FPS: {:>4.0f}\n", 1.f / m_flLastFrameTime));
    m_pFrameConsole->printLeft(FrameConsole::Cyan, fmt::format("Last frame time: {:>6.3f} ms\n", m_iLastFrameMicros / 1000.f));
    m_pFrameConsole->printLeft(FrameConsole::White, fmt::format("Pos: {} {} {}\n", m_Pos.x, m_Pos.y, m_Pos.z));
    m_pFrameConsole->printLeft(FrameConsole::White, fmt::format("Pitch: {}\n", m_Rot.x));
    m_pFrameConsole->printLeft(FrameConsole::White, fmt::format("Yaw: {}\n", m_Rot.y));
    m_pFrameConsole->printLeft(FrameConsole::White, fmt::format("Roll: {}\n", m_Rot.z));
    m_pFrameConsole->printLeft(FrameConsole::White, fmt::format("World surfaces: {}\n", stats.worldSurfaces));

    if (!isMouseInputEnabled()) {
        m_pFrameConsole->printLeft(FrameConsole::Red, "Mouse released\n");
    }

    m_pFrameConsole->printRight(FrameConsole::White, fmt::format("OpenGL {}.{}\n", GLVersion.major, GLVersion.minor));
    m_pFrameConsole->printRight(FrameConsole::White, fmt::format("{}\n", glGetString(GL_VENDOR)));
    m_pFrameConsole->printRight(FrameConsole::White, fmt::format("{}\n", glGetString(GL_RENDERER)));
}

bool App::isMouseInputEnabled() { return m_bGrabMouse; }

void App::setMouseInputEnabled(bool state) {
    if (state) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    m_bGrabMouse = state;
}

void App::checkKeys() {
    float speed = cam_speed.getValue() * m_flLastFrameTime;
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    if (m_DemoState != DemoState::Play) {
        auto fnMove = [&](float x, float y, float z) {
            m_Pos.x += x * speed;
            m_Pos.y += y * speed;
            m_Pos.z += z * speed;
        };

        // !!! In radians
        float pitch = glm::radians(m_Rot.x);
        float yaw = glm::radians(m_Rot.y);

        if (state[SDL_SCANCODE_LSHIFT]) {
            speed *= 5;
        }

        if (state[SDL_SCANCODE_W]) {
            fnMove(cos(yaw) * cos(pitch), sin(yaw) * cos(pitch), -sin(pitch));
        }

        if (state[SDL_SCANCODE_S]) {
            fnMove(-cos(yaw) * cos(pitch), -sin(yaw) * cos(pitch), sin(pitch));
        }

        if (state[SDL_SCANCODE_A]) {
            fnMove(-cos(yaw - glm::pi<float>() / 2), -sin(yaw - glm::pi<float>() / 2), 0);
        }

        if (state[SDL_SCANCODE_D]) {
            fnMove(cos(yaw - glm::pi<float>() / 2), sin(yaw - glm::pi<float>() / 2), 0);
        }
    }
}

void App::playDemo(const std::string &name) {
    stopDemo();

    try {
        std::string filename = "demos/" + name + ".dem";
        m_DemoReader.loadFromFile(filename);
        m_DemoReader.start(m_iTimeMicros);
        m_StatsWriter.start();
        logInfo("Playing {} - {}", filename, m_iTimeMicros);
        m_DemoState = DemoState::Play;
        m_bDrawDebugText = false;
    }
    catch (const std::exception &e) {
        logError("Error: {}", e.what());
    }
}

void App::recordDemo(const std::string &name) {
    stopDemo();

    try {
        m_DemoName = "demos/" + name + ".dem";
        m_DemoWriter.setTickRate(dem_tickrate.getValue());
        m_DemoWriter.start(m_iTimeMicros);
        logInfo("Recording to {}", m_DemoName);
        m_DemoState = DemoState::Record;
    } catch (const std::exception &e) { logError("Error: {}", e.what()); }
}

void App::stopDemo() {
    if (m_DemoState == DemoState::Record) {
        try {
            m_DemoWriter.finish(m_DemoName);
        } catch (const std::exception &e) {
            logError("stopDemo: {}", e.what());
        }
        logInfo("Demo recording stopped.");
    } else if (m_DemoState == DemoState::Play) {
        try {
            m_DemoReader.stop();
            m_StatsWriter.stop();
        } catch (const std::exception &e) { logError("stopDemo: {}", e.what()); }
        logInfo("Demo playing stopped.");
        m_bDrawDebugText = true;
    }

    m_DemoState = DemoState::None;
}

void App::loadMap(const std::string &name) {
    m_pRenderer->setLevel(nullptr);

    std::string path = "maps/" + name + ".bsp";    
    logInfo("Loading map {}", path);

    try {
        m_LoadedLevel.loadFromFile(path);
        m_pRenderer->setLevel(&m_LoadedLevel);

        m_Pos = {0.f, 0.f, 0.f};
        m_Rot = {0.f, 0.f, 0.f};

        logInfo("Map loaded");
    } catch (const std::exception &e) {
        logError("Failed to load map: {}", e.what());
        return;
    }
}

void App::updateViewportSize() {
    int wide, tall;
    SDL_GetWindowSize(m_pWindow, &wide, &tall);

    m_flAspectRatio = (float)wide / tall;
    glViewport(0, 0, wide, tall);
    m_pFrameConsole->updateViewportSize(wide, tall);
}

void fatalError(const std::string &msg) {
    AFW_DEBUG_BREAK();

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", msg.c_str(), nullptr);
    logFatal("Fatal Error: {}\n", msg);

    abort();
}

void App::StatsWriter::start() {
    m_iStartTime = App::get().m_iTimeMicros;
    m_Data.clear();
    m_Data.reserve(1024 * 1024); // 24 MB
}

void App::StatsWriter::add() {
    SavedData data;

    data.iTime = App::get().m_iTimeMicros - m_iStartTime;
    data.iFrameTime = App::get().m_iLastFrameMicros;
    data.iWPoly = (int)App::get().m_LastDrawStats.worldSurfaces;

    m_Data.push_back(data);
}

void App::StatsWriter::stop() {
    char buf[128];
    time_t t = std::time(nullptr);
    struct tm *time;
    time = std::localtime(&t);
    std::strftime(buf, sizeof(buf), "%Y_%m_%d__%H_%M_%S", time);

    std::ofstream file("stats/"s + buf + ".txt");
    AFW_ASSERT(!file.fail());

    for (auto &i : m_Data) {
        file << i.iTime << ' ' << i.iFrameTime << ' ' << i.iWPoly << '\n';
    }

    file.close();
}
