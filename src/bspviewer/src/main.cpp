#include <SDL2/SDL.h>
#include "app.h"

int realMain(int, char **) {
    try {
        App app;
        return app.run();
    }
    catch (const std::exception &e) {
        fatalError("Exception in main: "s + e.what());
        return 1;
    }
}

int main(int argc, char **argv) { return realMain(argc, argv); }