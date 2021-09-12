#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <gui_app_base/gui_components.h>
#include <gui_app_base/gui_app_base.h>
#include <gui_app_base/opengl_context.h>

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
    m_pWindow =
        SDL_CreateWindow(cfg.get<std::string>("win_title").c_str(), SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, cfg.get<int>("win_width"),
                         cfg.get<int>("win_height"), SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!m_pWindow) {
        app_fatalError("Failed to create window: {}", SDL_GetError());
    }
}

MainWindowComponent::~MainWindowComponent() {
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;
}

ImGuiComponent::ImGuiComponent() {
    SDL_Window *pWindow = MainWindowComponent::get().getWindow();
    SDL_GLContext glContext = OpenGLContext::get().getContext();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // ImGuiIO &io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(pWindow, glContext);
    ImGui_ImplOpenGL3_Init();

    setupScale();
}

void ImGuiComponent::beginTick() {
    appfw::Prof prof("ImGui Begin Frame");
    SDL_Window *pWindow = MainWindowComponent::get().getWindow();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(pWindow);
    ImGui::NewFrame();
}

void ImGuiComponent::endTick() {
    appfw::Prof prof("ImGui End Frame");
    ImGui::EndFrame();
}

void ImGuiComponent::render() {
    appfw::Prof prof("ImGui");
    glEnable(GL_FRAMEBUFFER_SRGB);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glDisable(GL_FRAMEBUFFER_SRGB);
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
    float guiScale = config.getItem("gui").get<float>("imgui_scale", 0.0f);
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
