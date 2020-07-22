#include <iostream>
#include <appfw/init.h>
#include <appfw/services.h>
#include <appfw/compiler.h>
#include <glad/glad.h>

#include "app.h"

static ConCommand quit_cmd("quit", "Exits the app", [](auto &) { App::get().quit(); });

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
                // TODO: Adjust viewport
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
    case SDL_QUIT: {
        quit();
        break;
    }
    }
}

void App::draw() {
    glClearColor(0, 0.5f, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(m_pWindow);
}

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
    m_pWindow = SDL_CreateWindow(
        "BSP Viewer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1600, 900,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

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

    logInfo("OpenGL Version {}.{} loaded", GLVersion.major, GLVersion.minor);

    if (!GLAD_GL_VERSION_3_3) {
        fatalError(fmt::format("At least OpenGL 3.3 is required. Maximum is {}.{}", GLVersion.major, GLVersion.minor));
    }
}

App::~App() {
    AFW_ASSERT(m_sSingleton);

    SDL_GL_DeleteContext(m_GLContext);
    m_GLContext = nullptr;

    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;

	appfw::init::shutdown();

    SDL_Quit();

    m_sSingleton = nullptr;
}

int App::run() {
	m_bIsRunning = true;

	while (m_bIsRunning) {
		tick();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
	}

	return m_iReturnCode;
}

//static ConVar<bool> print_shit("print_shit", false, "sdfsdfsdf");

void App::tick() {

	/*if (print_shit.getValue()) {
        static int tick = 0;

        if (tick == 30) {
            static int i = 0;
            logDebug("Test output {}!", ++i);
            tick = 0;
        }

        tick++;
    }*/

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        handleSDLEvent(event);
    }

    draw();

	appfw::init::mainLoopTick();
}

void fatalError(const std::string &msg) {
    AFW_DEBUG_BREAK();

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", msg.c_str(), nullptr);
    logFatal("Fatal Error: {}\n", msg);

    abort();
}