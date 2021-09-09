#include <appfw/init.h>
#include <app_base/app_base.h>

int main(int argc, char **argv) {
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
