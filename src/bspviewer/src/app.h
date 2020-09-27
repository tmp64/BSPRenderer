#ifndef APP_H
#define APP_H
#include <SDL2/SDL.h>
#include <appfw/timer.h>
#include <appfw/utils.h>
#include <bsp/level.h>
#include <glm/glm.hpp>
#include <renderer/base_renderer.h>
#include "demo.h"
#include "input_system.h"
#include "dev_console_dialog.h"

class FrameConsole;
class BaseRenderer;

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
    void drawDebugText();

    /**
     * Returns whether mouse is grabbed by the app.
     */
    bool isMouseInputEnabled();

    void checkKeys();

    void mouseMoved(int xrel, int yrel);

    void playDemo(const std::string &name);
    void recordDemo(const std::string &name);
    void stopDemo();

    void setDrawDebugTextEnabled(bool state);
    void toggleConsole();

    inline bool isDrawDebugTextEnabled() { return m_bDrawDebugText; }

private:
    enum class DemoState {
        None = 0,
        Record,
        Play
    };

    // Time since start (microseconds)
    // Frame time (microseconds)
    // World polys
    class StatsWriter {
    public:
        void start();
        void add();
        void stop();
    
    private:
        struct SavedData {
            long long iTime = 0;
            long long iFrameTime = 0;
            int iWPoly = 0;
        };

        long long m_iStartTime = 0;
        std::vector<SavedData> m_Data;
    };

    bool m_bIsRunning = false;
    int m_iReturnCode = 0;

    appfw::Timer m_Timer;
    unsigned m_iLastFrameMicros = 0;
    long long m_iTimeMicros = 0;
    float m_flLastFrameTime = 0;
    SDL_Window *m_pWindow = nullptr;
    SDL_GLContext m_GLContext = nullptr;

    BaseRenderer *m_pRenderer = nullptr;
    bsp::Level m_LoadedLevel;
    BaseRenderer::DrawStats m_LastDrawStats;
    glm::vec3 m_Pos = {0.f, 0.f, 0.f};
    glm::vec3 m_Rot = {0.f, 0.f, 0.f};
    float m_flAspectRatio = 1.f;

    bool m_bDrawDebugText = true;

    DemoState m_DemoState = DemoState::None;
    DemoReader m_DemoReader;
    DemoWriter m_DemoWriter;
    StatsWriter m_StatsWriter;
    std::string m_DemoName;

    InputSystem *m_pInputSystem = nullptr;
    DevConsoleDialog *m_pDevConsole = nullptr;
    bool m_bIsConsoleVisible = true;

    App();
    ~App();
    void initDearImGui();
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
