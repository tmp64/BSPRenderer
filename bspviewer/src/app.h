#ifndef APP_H
#define APP_H
#include <SDL2/SDL.h>
#include <appfw/utils.h>

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
     * Updates window contents.
     */
    void draw();

private:
    bool m_bIsRunning = false;
    int m_iReturnCode = 0;

    SDL_Window *m_pWindow = nullptr;
    SDL_GLContext m_GLContext = nullptr;

    App();
    ~App();
    int run();
    void tick();

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
