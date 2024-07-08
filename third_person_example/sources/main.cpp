#include <simulant/simulant.h>

#include "scenes/game.h"

using namespace smlt;

class Third_Person_Example:
    public Application {

public:
    Third_Person_Example(const AppConfig& config):
        Application(config) {}

    bool init() {
        // Register screens here
        scenes->register_scene<smlt::scenes::Splash>("main", "app");
        scenes->register_scene<GameScene>("app");

        return true;
    }
};

extern "C" {

#ifdef __ANDROID__

#include <SDL_main.h>
#include <gpg/gpg.h>

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    gpg::AndroidInitialization::JNI_OnLoad(vm);
    return JNI_VERSION_1_6;
}

    int SDL_main(int argc, char* argv[])
#else
    int main(int argc, char* argv[])
#endif
{
    AppConfig config;
    config.title = "third_person_example";
    config.search_paths.push_back("assets");

    // For Flatpak
    config.search_paths.push_back(kfs::exe_dirname() + "/assets");

    // Dreamcast only
    config.search_paths.push_back("/cd/assets");

#ifndef NDEBUG
    config.log_level = smlt::LOG_LEVEL_DEBUG;
#endif

    Third_Person_Example app(config);
    return app.run();
}

}
