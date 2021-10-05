#ifndef LEVEL_LOADER_H
#define LEVEL_LOADER_H
#include <string_view>
#include <appfw/utils.h>
#include "assets/level_asset.h"

//! A level loader. Loads the level and renderer, displays the progress dialog.
//! While it exists, a level is being loaded. Loading can't be aborted (only blocked until it finishes).
//! The destructior will block if it's still loading.
class LevelLoader : appfw::NoMove {
public:
    LevelLoader(std::string_view path);
    ~LevelLoader();

    //! Updates the process. On error throws.
    //! @returns true when loading has finished
    bool tick();

    //! Returns the loaded level.
    inline LevelAssetRef getLevel() { return m_pLevel; }

private:
    enum class Status
    {
        Level,
        Renderer,
        Finished
    };

    Status m_Status;
    LevelAssetFuture m_Future;
    LevelAssetRef m_pLevel;
};

#endif
