#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <gui_app_base/gui_components.h>
#include <gui_app_base/gui_app_base.h>
#include <gui_app_base/opengl_context.h>

static constexpr int WIN_POS_UNDEFINED = std::numeric_limits<int>::min();
static ConfigItem<int> cfg_window_width("window_width", 640, "Width of the main window.");
static ConfigItem<int> cfg_window_height("window_height", 480, "Height of the main window.");
static ConfigItem<int> cfg_window_x("window_x", WIN_POS_UNDEFINED,
                                    "Position X of the main window.");
static ConfigItem<int> cfg_window_y("window_y", WIN_POS_UNDEFINED,
                                    "Position Y of the main window.");

SDLComponent::SDLComponent() {
    SDL_Init(SDL_INIT_EVERYTHING);
}

SDLComponent::~SDLComponent() {
    SDL_Quit();
}

MainWindowComponent::MainWindowComponent() {
    AppConfig::Item cfg = AppBase::getBaseInstance().getConfig().getItem("gui");

    // Set up OpenGL
    OpenGLContext::setupWindowAttributes();

    // Create window
    int wide = cfg.get<int>("win_width");
    int tall = cfg.get<int>("win_height");
    cfg_window_width.setDefaultValue(wide);
    cfg_window_height.setDefaultValue(tall);
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    m_pWindow = SDL_CreateWindow(cfg.get<std::string>("win_title").c_str(), SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, wide, tall, flags);

    if (!m_pWindow) {
        app_fatalError("Failed to create window: {}", SDL_GetError());
    }
}

MainWindowComponent::~MainWindowComponent() {
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
}

void MainWindowComponent::lateInit() {
    int wide = std::clamp(cfg_window_width.getValue(), 1, 16384);
    int tall = std::clamp(cfg_window_height.getValue(), 1, 16384);

    SDL_SetWindowSize(m_pWindow, wide, tall);

    int x, y;

    if (cfg_window_x.getValue() == WIN_POS_UNDEFINED) {
        x = SDL_WINDOWPOS_CENTERED;
    } else {
        x = std::clamp(cfg_window_x.getValue(), -65536, 65535);
    }

    if (cfg_window_y.getValue() == WIN_POS_UNDEFINED) {
        y = SDL_WINDOWPOS_CENTERED;
    } else {
        y = std::clamp(cfg_window_y.getValue(), -65536, 65535);
    }

    SDL_SetWindowPosition(m_pWindow, x, y);
}

void MainWindowComponent::saveWindowSize() {
    int wide, tall;
    SDL_GetWindowSize(m_pWindow, &wide, &tall);

    if (cfg_window_width.getValue() != wide) {
        cfg_window_width.setValue(wide);
    }

    if (cfg_window_height.getValue() != tall) {
        cfg_window_height.setValue(tall);
    }
}

void MainWindowComponent::saveWindowPos() {
    int x, y;
    SDL_GetWindowPosition(m_pWindow, &x, &y);

    if (cfg_window_x.getValue() != x) {
        cfg_window_x.setValue(x);
    }

    if (cfg_window_y.getValue() != y) {
        cfg_window_y.setValue(y);
    }
}

ImGuiComponent::ImGuiComponent() {
    SDL_Window *pWindow = MainWindowComponent::get().getWindow();
    SDL_GLContext glContext = OpenGLContext::get().getContext();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look
    // identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(pWindow, glContext);
    ImGui_ImplOpenGL3_Init();

    setupScale();
}

void ImGuiComponent::beginTick() {
    appfw::Prof prof("ImGui Begin Frame");
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ImGuiComponent::endTick() {
    appfw::Prof prof("ImGui End Frame");
    ImGui::EndFrame();
}

void ImGuiComponent::render() {
    appfw::Prof prof("ImGui");
    ImGuiIO &io = ImGui::GetIO();

    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it
    // easier to paste this code elsewhere.
    //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context)
    //  directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_Window *backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
}

ImFont *ImGuiComponent::loadFont(std::string_view filepath, float sizePixels) {
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

    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(data, size, sizePixels * m_flScale);
}

ImFont *ImGuiComponent::loadFontOrDefault(std::string_view filepath, float sizePixels) {
    ImFont *font = loadFont(filepath, sizePixels);
    
    if (font) {
        return font;
    } else {
        ImFontConfig config;
        config.SizePixels = sizePixels * m_flScale;
        return ImGui::GetIO().Fonts->AddFontDefault(&config);
    }
}

void ImGuiComponent::setupScale() {
    AppConfig &config = AppBase::getBaseInstance().getConfig();
    float guiScale = 1.0f;

    if (getCommandLine().doesArgHaveValue("--gui-scale")) {
        guiScale = getCommandLine().getArgFloat("--gui-scale");
    } else {
        guiScale = config.getItem("gui").get<float>("imgui_scale", 0.0f);
    }

    guiScale = std::max(guiScale, 0.0f);

    if (guiScale == 0.0f) {
        guiScale = 1.0f;
        float dpi = 0.0f;

        if (SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr) == 0) {
            if (dpi > NORMAL_DPI) {
                guiScale = dpi / NORMAL_DPI;
                printd("Display DPI: {}, UI scale: {}", dpi, guiScale);
            } else if (dpi < NORMAL_DPI) {
                printw("DPI is lower than normal DPI ({} < {}), UI not scaled", dpi, NORMAL_DPI);
            }
        } else {
            printw("Failed to get display DPI: {}", SDL_GetError());
        }
    }

    m_flScale = guiScale;

    std::string fontPath = config.getItem("gui").get<std::string>("imgui_font_path", "");
    float fontSize = config.getItem("gui").get<float>("imgui_font_size", 13.0f);
    loadFontOrDefault(fontPath, fontSize);

    ImGui::GetStyle().ScaleAllSizes(guiScale);
}
