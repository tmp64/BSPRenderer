#ifndef HLVIEWER_ASSET_LOADER_H
#define HLVIEWER_ASSET_LOADER_H
#include <queue>
#include <hlviewer/assets/asset_manager.h>
#include <hlviewer/assets/level_asset.h>

class AssetLoader {
public:
    //! Maximum run time in us.
    static constexpr unsigned MAX_TIME_MICROS = 32'000;

    //! @returns whether all requested assets are loaded.
    inline bool isFinished() { return m_Tasks.empty(); }

    //! @returns the name of the last item.
    inline const std::string &getLastItemName() const { return m_LastItemName; }

    //! @returns total item count.
    inline int getTotalItemCount() const { return m_iTotalItems; }

    //! @returns finished item count.
    inline int getFinishedItemCount() const { return m_iFinishedItems; }

    //! Processes the asset queue. If any asset fails to load, throws the exception.
    //! @param  block   If false, method will only run for limited time.
    void processQueue(bool block = false);

    //! Begins loading of a level.
    //! When isFinished() == true, all textures will have been loaded.
    LevelAsset loadLevel(std::string_view path);

private:
    std::queue<std::function<void()>> m_Tasks;
    int m_iTotalItems = 0;
    int m_iFinishedItems = 0;
    std::string m_LastItemName;
};

#endif
