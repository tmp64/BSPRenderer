#ifndef APP_H
#define APP_H
#include <SDL2/SDL.h>
#include <appfw/timer.h>
#include <appfw/utils.h>
#include <bsp/level.h>
#include <glm/glm.hpp>
#include <renderer/base_renderer.h>

class FrameConsole;
class PolygonRenderer;

class App : appfw::utils::NoCopy {
public:
    static App &get();

    /**
     * Gracefullt quit the app.
     * @param   code    Exit code
     */
    void quit(int code = 0);

    /**
     * Handles an SDL event.
     * Can be used to generate key presses.
     */
    void handleSDLEvent(SDL_Event event);

    /**
     * Load a .bsp map
     * @param   name    Name of the map, without maps/ and .bsp.
     */
    void loadMap(const std::string &name);

    /**
     * Updates window contents.
     */
    void draw();

    /**
     * Draw text info
     */
    void drawDebugText(const BaseRenderer::DrawStats &stats);

    /**
     * Returns whether mouse is grabbed by the app.
     */
    bool isMouseInputEnabled();

    /**
     * Enables/disables mouse input.
     */
    void setMouseInputEnabled(bool state);

    void checkKeys();

private:
    bool m_bIsRunning = false;
    int m_iReturnCode = 0;

    appfw::Timer m_Timer;
    unsigned m_iLastFrameMicros = 0;
    float m_flLastFrameTime = 0;
    SDL_Window *m_pWindow = nullptr;
    SDL_GLContext m_GLContext = nullptr;

    FrameConsole *m_pFrameConsole = nullptr;
    PolygonRenderer *m_pRenderer = nullptr;
    bsp::Level m_LoadedLevel;
    glm::vec3 m_Pos = {0.f, 0.f, 0.f};
    glm::vec3 m_Rot = {0.f, 0.f, 0.f};
    float m_flAspectRatio = 1.f;

    bool m_bGrabMouse = false;

    App();
    ~App();
    int run();
    void tick();
    void updateViewportSize();

    static inline App *m_sSingleton = nullptr;
    friend int realMain(int, char**);
};

inline App &App::get() { return *m_sSingleton; }

using namespace std::literals::string_literals;

/**
 * Shows a fatal error and forcefully closes the app.
 */
[[noreturn]] void fatalError(const std::string &msg);

#endif
