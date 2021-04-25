#include <iostream>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include <appfw/init.h>
#include <appfw/services.h>
#include <appfw/dbg.h>
#include <renderer/shader_manager.h>
#include <renderer/material_manager.h>
#include <gui_app/utils.h>
#include <gui_app/gui_app_base.h>

SDLAppComponent::SDLAppComponent() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Fatal error!\n";
        std::cerr << "Failed to initialize SDL2.\n";
        std::cerr << SDL_GetError() << "\n";
        std::exit(-1);
    }

    m_sIsInit = true;
}

SDLAppComponent::~SDLAppComponent() {
    m_sIsInit = false;
    SDL_Quit();
}

AppFWAppComponent::AppFWAppComponent() {
    using namespace appfw::init;
    init();
}

AppFWAppComponent::~AppFWAppComponent() { appfw::init::shutdown(); }

void AppFWAppComponent::tick() { appfw::init::mainLoopTick(); }

FileSystemAppComponent::FileSystemAppComponent() {
    m_BaseAppPath = app_getInitInfo().baseAppPath;

    if (m_BaseAppPath.empty()) {
        m_BaseAppPath = fs::current_path();
    }

    logInfo("Base app path: {}", m_BaseAppPath.u8string());

    // Add paths
    getFileSystem().addSearchPath(m_BaseAppPath, "base");
}

AppConfigInitAppComponent::AppConfigInitAppComponent(AppConfig &cfg) {
    cfg.mountFilesystem();
    cfg.executeCommands();
}

OpenGLAppComponent::OpenGLAppComponent() {
    if (SDL_GL_LoadLibrary(nullptr)) {
        app_fatalError("Failed to load OpenGL library: {}", SDL_GetError());
    }

    if (appfw::platform::isDebugBuild()) {
        logDebug("OpenGL: Using debug context.");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }
}

OpenGLAppComponent::~OpenGLAppComponent() {}

MainWindowAppComponent::MainWindowAppComponent(AppConfig &config) {
    AppConfig::Item cfg = config.getItem("gui");

    // Create window
    m_pWindow = SDL_CreateWindow(cfg.get<std::string>("win_title").c_str(), SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, cfg.get<int>("win_width"), cfg.get<int>("win_height"),
                                 SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!m_pWindow) {
        app_fatalError("Failed to create window: {}", SDL_GetError());
    }

    // Create OpenGL context
    m_GLContext = SDL_GL_CreateContext(m_pWindow);

    if (!m_GLContext) {
        app_fatalError("Failed to create OpenGL context: ", SDL_GetError());
    }

    // Init OpenGL
    static bool bIsGladLoaded = false;

#ifdef GLAD_OPENGL
    if (!gladLoadGLLoader([](const char *name) { return SDL_GL_GetProcAddress(name); })) {
        app_fatalError("Failed load OpenGL functions.");
    }

    if (!GLAD_GL_VERSION_3_2) {
        app_fatalError("At least OpenGL 3.2 is required. Available {}.{}", GLVersion.major, GLVersion.minor);
    }
#elif defined(GLAD_OPENGL_ES)
    if (!gladLoadGLES2Loader([](const char *name) { return SDL_GL_GetProcAddress(name); })) {
        app_fatalError("Failed load OpenGL ES functions.");
    }

    if (!GLAD_GL_ES_VERSION_3_1) {
        app_fatalError("At least OpenGL ES 3.1 is required. Available {}.{}", GLVersion.major, GLVersion.minor);
    }
#else
#error "Unknown GL type"
#endif

    glad_set_post_callback(gladPostCallback);

    logInfo("OpenGL Version {}.{} loaded", GLVersion.major, GLVersion.minor);
    bIsGladLoaded = true;
}

MainWindowAppComponent::~MainWindowAppComponent() {
    // Remove GL context
    SDL_GL_DeleteContext(m_GLContext);
    m_GLContext = nullptr;

    // Destroy window
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
}

static ConVar<bool> gl_break_on_error("gl_break_on_error", true, "Break in debugger on OpenGL error");

void MainWindowAppComponent::gladPostCallback(const char *name, void *, int, ...) {
    GLenum errorCode = glad_glGetError();

    if (errorCode != GL_NO_ERROR) {
        logError("OpenGL Error: {} (0x{:X}) in {}", getGlErrorString(errorCode), (int)errorCode, name);
        if (gl_break_on_error.getValue()) {
            AFW_DEBUG_BREAK();
        }
    }
}

const char *MainWindowAppComponent::getGlErrorString(GLenum errorCode) {
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
        return "< unknown >";
    }
}

ImGuiAppComponent::ImGuiAppComponent(MainWindowAppComponent &window) {
    m_pWindow = &window;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window.getWindow(), window.getContext());
    ImGui_ImplOpenGL3_Init();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use
    // ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application
    // (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling
    // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double
    // backslash \\ !
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL,
    // io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != NULL);}
}

ImGuiAppComponent::~ImGuiAppComponent() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiAppComponent::newTick() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(m_pWindow->getWindow());
    ImGui::NewFrame();
}

void ImGuiAppComponent::draw() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

RendererSystemAppComponent::RendererSystemAppComponent() {
    ShaderManager::get().init();
    MaterialManager::get().init();
}

RendererSystemAppComponent::~RendererSystemAppComponent() {
    MaterialManager::get().shutdown();
    ShaderManager::get().shutdown();
}

void RendererSystemAppComponent::tick() { MaterialManager::get().tick(); }
