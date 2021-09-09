#include "SDL.h"
#include <appfw/init.h>
#include <app_base/app_base.h>

static void guiFatalErrorHandler(const std::string &msg) noexcept {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", msg.c_str(), nullptr);
}

// main is redefined as SDL_main and called from SDL2
int main(int argc, char **argv) {
    app_setFatalErrorHandler(&guiFatalErrorHandler);

    try {
        appfw::InitOptions opts;
        opts.setArgs(argc, argv);
        appfw::InitComponent appfw(opts);

        std::unique_ptr<AppBase> app = app_createSingleton();
        return app->run();
    } catch (const std::exception &e) {
        app_fatalError("Exception in main: {}", e.what());
        return 1;
    }
}
