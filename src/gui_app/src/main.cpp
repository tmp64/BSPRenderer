#include "SDL.h"
#include <gui_app/gui_app_base.h>

int app_runmain(int, char **) {
    SDLAppComponent sdl;

    try {
        std::unique_ptr<GuiAppBase> app = app_createSingleton();
        return app->run();
    } catch (const std::exception &e) {
        app_fatalError("Exception in main: {}", e.what());
        return 1;
    }
}
