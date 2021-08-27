#include <iostream>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include <appfw/init.h>
#include <appfw/appfw.h>
#include <appfw/dbg.h>
#include <renderer/shader_manager.h>
#include <renderer/material_manager.h>
#include <gui_app/utils.h>
#include <gui_app/gui_app_base.h>

#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT 2
#endif

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

void AppFWAppComponent::tick() {
    appfw::Prof prof("appfw");
    appfw::mainLoopTick();
}

FileSystemAppComponent::FileSystemAppComponent() {
    m_BaseAppPath = app_getInitInfo().baseAppPath;

    if (m_BaseAppPath.empty()) {
        m_BaseAppPath = fs::current_path();
    }

    printi("Base app path: {}", m_BaseAppPath.u8string());

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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    int contextFlags = 0;

    bool bNoDebug = getCommandLine().isFlagSet("--gl-no-debug");
    bool bDebug = appfw::isDebugBuild() || getCommandLine().isFlagSet("--gl-debug");

    if (bDebug && !bNoDebug) {
        contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);
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

    if (!GLAD_GL_VERSION_3_3) {
        app_fatalError("At least OpenGL 3.3 is required. Available {}.{}", GLVersion.major, GLVersion.minor);
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

    printi("OpenGL: Version {}.{} loaded", GLVersion.major, GLVersion.minor);
    printi("GL_VENDOR: {}", glGetString(GL_VENDOR));
    printi("GL_RENDERER: {}", glGetString(GL_RENDERER));
    printi("GL_VERSION: {}", glGetString(GL_VERSION));

    int contextFlags = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &contextFlags);
    bool bDebugRequested = contextFlags & SDL_GL_CONTEXT_DEBUG_FLAG;

    int glProfileMask = 0;
    int glContextFlags = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &glProfileMask);
    glGetIntegerv(GL_CONTEXT_FLAGS, &glContextFlags);

    if (glProfileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) {
        printw("OpenGL: Requested core profile, got compatibility instead");
    }

    if (glContextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        if (GLAD_GL_ARB_debug_output) {
            printd("OpenGL: Using debug context");
        } else {
            printw("OpenGL: Got debug context but ARB_debug_output is not supported");
        }
    } else if (bDebugRequested) {
        if (GLAD_GL_ARB_debug_output) {
            printw("OpenGL: Debug context NOT requested but ARB_debug_output is available");
        } else {
            printw("OpenGL: Debug context requested but not available");
        }
    }

    if (GLAD_GL_ARB_debug_output) {
        glDebugMessageCallbackARB(&debugMsgCallback, nullptr);
    }

    bIsGladLoaded = true;
}

MainWindowAppComponent::~MainWindowAppComponent() {
    // Remove debug callback
    if (GLAD_GL_ARB_debug_output) {
        glDebugMessageCallbackARB(nullptr, nullptr);
    }

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
        printe("OpenGL Error: {} (0x{:X}) in {}", getGlErrorString(errorCode), (int)errorCode, name);
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

void MainWindowAppComponent::debugMsgCallback(GLenum source, GLenum type, GLuint id,
                                              GLenum severity, GLsizei length,
                                              const GLchar *message, const void *) {
    std::string_view sourceStr;
    std::string_view typeStr;
    std::string_view severityStr;
    ConMsgType conMsgType = ConMsgType::Warn;

    //----------------------------------------
    switch (source) {
    case GL_DEBUG_SOURCE_API_ARB: {
        sourceStr = "GL API";
        break;
    }
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: {
        sourceStr = "GL API";
        break;
    }
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: {
        sourceStr = "Shader Compiler";
        break;
    }
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: {
        sourceStr = "Third-Party";
        break;
    }
    case GL_DEBUG_SOURCE_APPLICATION_ARB: {
        sourceStr = "Application";
        break;
    }
    case GL_DEBUG_SOURCE_OTHER_ARB: {
        sourceStr = "Other";
        break;
    }
    default: {
        sourceStr = "Unknown";
        break;
    }
    }

    //----------------------------------------
    switch (type) {
    case GL_DEBUG_TYPE_ERROR_ARB: {
        typeStr = "Error";
        break;
    }
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: {
        typeStr = "Deprecated Behavior";
        break;
    }
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: {
        typeStr = "Undefined Behavior";
        break;
    }
    case GL_DEBUG_TYPE_PORTABILITY_ARB: {
        typeStr = "Portability";
        break;
    }
    case GL_DEBUG_TYPE_PERFORMANCE_ARB: {
        typeStr = "Performance";
        break;
    }
    case GL_DEBUG_TYPE_OTHER_ARB: {
        typeStr = "Other";
        break;
    }
    default: {
        typeStr = "Unknown";
        break;
    }
    }

    //----------------------------------------
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH_ARB: {
        severityStr = "High";
        conMsgType = ConMsgType::Error;
        break;
    }
    case GL_DEBUG_SEVERITY_MEDIUM_ARB: {
        severityStr = "Medium";
        break;
    }
    case GL_DEBUG_SEVERITY_LOW_ARB: {
        severityStr = "Low";
        break;
    }
    default: {
        severityStr = "Unknown";
        break;
    }
    }

    //----------------------------------------
    printtype(conMsgType, "OpenGL: {}: {}", id, std::string_view(message, length));
    printtype(conMsgType, "OpenGL: Source: {}", sourceStr);
    printtype(conMsgType, "OpenGL: Type: {}", typeStr);
    printtype(conMsgType, "OpenGL: Severity: {}", severityStr);
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
    appfw::Prof prof("ImGui New Frame");
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(m_pWindow->getWindow());
    ImGui::NewFrame();
}

void ImGuiAppComponent::draw() {
    appfw::Prof prof("ImGui");
    glEnable(GL_FRAMEBUFFER_SRGB);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glDisable(GL_FRAMEBUFFER_SRGB);
}

ImFont *ImGuiAppComponent::loadFont(std::string_view filepath, float size_pixels) {
    char *data = nullptr;
    int size = 0;

    try {
        std::ifstream file;
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file.open(getFileSystem().findExistingFile(filepath), std::ifstream::binary);

        size = (int)appfw::getFileSize(file);
        data = (char *)IM_ALLOC(size);
        file.read(data, size);
    } catch (const std::exception &e) {
        printe("ImGui: Failed to open font {}: {}", filepath, e.what());
        IM_FREE(data);
        return nullptr;
    }

    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(data, size, size_pixels);
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
