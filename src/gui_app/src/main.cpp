#include "SDL.h"
#include <appfw/appfw.h>
#include <appfw/init.h>
#include <gui_app/gui_app_base.h>

int app_runmain(int argc, char **argv) {
    SDLAppComponent sdl;
    appfw::InitOptions opts;
    opts.setArgs(argc, argv);
    appfw::InitComponent appfw(opts);

    try {
        std::unique_ptr<GuiAppBase> app = app_createSingleton();
        return app->run();
    } catch (const std::exception &e) {
        app_fatalError("Exception in main: {}", e.what());
        return 1;
    }
}
