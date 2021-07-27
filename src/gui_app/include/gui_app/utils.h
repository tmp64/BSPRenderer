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
class SDLAppComponent : appfw::NoCopy {
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
class AppFWAppComponent : appfw::NoCopy {
public:
    void tick();
};

/**
 * Initializes appfw::FileSystem.
 */
class FileSystemAppComponent : appfw::NoCopy {
public:
    FileSystemAppComponent();

    fs::path m_BaseAppPath;
};

class AppConfigInitAppComponent : appfw::NoCopy {
public:
    AppConfigInitAppComponent(AppConfig &cfg);
};

/**
 * SDL2 OpenGL RAII wrapper.
 */
class OpenGLAppComponent : appfw::NoCopy {
public:
    OpenGLAppComponent();
    ~OpenGLAppComponent();
};

/**
 * SDL2 window and OpenGL context RAII wrapper.
 */
class MainWindowAppComponent : appfw::NoCopy {
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
    static void APIENTRY debugMsgCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                          GLsizei length, const GLchar *message,
                                          const void *userParam);
};

/**
 * ImGui RAII wrapper.
 */
class ImGuiAppComponent : appfw::NoCopy {
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
class RendererSystemAppComponent : appfw::NoCopy {
public:
    RendererSystemAppComponent();
    ~RendererSystemAppComponent();

    void tick();
};

#endif
