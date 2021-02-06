#ifndef GUI_APP_UTILS_H
#define GUI_APP_UTILS_H
#include <SDL.h>
#include <glad/glad.h>
#include <appfw/utils.h>
#include <appfw/filesystem.h>

class AppConfig;

/**
 * SDL2 RAII wrapper.
 */
class SDLAppComponent : appfw::utils::NoCopy {
public:
    SDLAppComponent();
    ~SDLAppComponent();

    static inline bool isInitialized() { return m_sIsInit; }

private:
    static inline bool m_sIsInit = false;
};

/**
 * AppFW RAII wrapper.
 */
class AppFWAppComponent : appfw::utils::NoCopy {
public:
    AppFWAppComponent();
    ~AppFWAppComponent();

    void tick();
};

/**
 * Initializes appfw::FileSystem.
 */
class FileSystemAppComponent : appfw::utils::NoCopy {
public:
    FileSystemAppComponent();

    fs::path m_BaseAppPath;
};

/**
 * SDL2 OpenGL RAII wrapper.
 */
class OpenGLAppComponent : appfw::utils::NoCopy {
public:
    OpenGLAppComponent();
    ~OpenGLAppComponent();
};

/**
 * SDL2 window and OpenGL context RAII wrapper.
 */
class MainWindowAppComponent : appfw::utils::NoCopy {
public:
    MainWindowAppComponent(AppConfig &config);
    ~MainWindowAppComponent();

    inline SDL_Window *getWindow() { return m_pWindow; }
    inline SDL_GLContext getContext() { return m_GLContext; }

private:
    SDL_Window *m_pWindow = nullptr;
    SDL_GLContext m_GLContext = nullptr;

    static void gladPostCallback(const char *name, void *, int, ...);
    static const char *getGlErrorString(GLenum errorCode);
};

/**
 * ImGui RAII wrapper.
 */
class ImGuiAppComponent : appfw::utils::NoCopy {
public:
    ImGuiAppComponent(MainWindowAppComponent &window);
    ~ImGuiAppComponent();

    /**
     * Called at the beginning of each tick.
     */
    void newTick();

    /**
     * Draws the UI.
     */
    void draw();

private:
    MainWindowAppComponent *m_pWindow = nullptr;
};

/**
 * Renderer system RAII wrapper.
 */
class RendererSystemAppComponent : appfw::utils::NoCopy {
public:
    RendererSystemAppComponent();
    ~RendererSystemAppComponent();
};

#endif
