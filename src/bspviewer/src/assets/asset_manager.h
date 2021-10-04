#ifndef ASSETS_ASSET_MANAGER_H
#define ASSETS_ASSET_MANAGER_H
#include <atomic>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>
#include <appfw/unique_function.h>
#include <app_base/app_component.h>
#include "level_asset.h"
#include "wad_asset.h"
#include "asset_future.h"

class AssetManager : public AppComponentBase<AssetManager> {
public:
    AssetManager();
    ~AssetManager();

    void lateTick() override;

    //! Loads a level along with all its textures.
    LevelAssetFuture loadLevel(std::string_view path);

    //! Loads a WAD file with textures. Textures are loaded on demand.
    std::future<WADAssetRef> loadWADFile(std::string_view path);

    //! Finds a material by name or returns null reference.
    WADMaterialAssetRef findMaterialByName(const char *szName);

private:
    using AsyncTask = appfw::unique_function<void()>;

    //! Exception thrown when asset manager is shutting down
    class ShutdownException : public std::runtime_error {
        ShutdownException()
            : std::runtime_error("Asset manager is shutting down") {}
    };

    // Worker thread stuff
    std::thread m_WorkerThread;
    std::queue<AsyncTask> m_WorkerTasks;
    std::mutex m_WorkerTasksMutex;
    std::condition_variable m_WorkerTasksCondVar;
    std::atomic_bool m_bWorkerIsRunning = false;

    // Worker -> main thread execution
    std::queue<AsyncTask> m_MainThreadTasks;
    std::mutex m_MainThreadTasksMutex;

    // Levels
    std::mutex m_LevelMutex;
    std::list<LevelAsset> m_LevelList;

    // WADs
    std::mutex m_WADMutex;
    std::list<WADAsset> m_WADList;

    //! Loads a WAD file
    WADAssetRef loadWADInternal(std::string_view path);

    //! Adds an async task to the end of the queue.
    //! Can be called from any thread.
    void addWorkerTask(AsyncTask &&task);

    //! Adds an async task to the end of the queue for main thread.
    //! Can be called from any thread.
    void addMainThreadTask(AsyncTask &&task);

    //! The worker thread
    void workerThread();

    //! Causes a fake delay
    void fakeDelay();

    //! Waits for a future value and returns it. If app is shutdown during waiting, throws ShutdownException.
    template <typename T>
    inline auto waitOnFuture(T &future) {
        auto var = future.get();

        if (!m_bWorkerIsRunning) {
            throw ShutdownException();
        }

        return var;
    }

    friend class LevelAsset;
    friend class WADAsset;
};

#endif
