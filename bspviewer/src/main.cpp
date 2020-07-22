#include <SDL2/SDL.h>
#include "app.h"

int realMain(int, char **) {
    App app;
    return app.run();
}

int main(int argc, char **argv) { return realMain(argc, argv); }