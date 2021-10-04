#include <appfw/appfw.h>
#include <app_base/app_base.h>
#include "asset_manager.h"

ConVar<float> asset_fake_delay("asset_fake_delay", 0,
                               "Adds an additional delay (seconds) when loading assets");

ConVar<int> asset_max_tick("asset_max_tick", 25, "Maximum ms time to load assets in one tick");

AssetManager::AssetManager() {
    setTickEnabled(true);
    m_bWorkerIsRunning = true;
    m_WorkerThread = std::thread([&]() { workerThread(); });
}

AssetManager::~AssetManager() {
    // FIXME: This may deadlock if textures are uploading
    m_bWorkerIsRunning = false;
    m_WorkerTasksCondVar.notify_one();
    m_WorkerThread.join();

    [[maybe_unused]] bool bLeaksFound = false;

    // Unload levels
    if constexpr (appfw::isDebugBuild()) {
        for (LevelAsset &level : m_LevelList) {
            if (level.m_uReferenceCount != 0) {
                bLeaksFound = true;
                printw("Level leaked: {}", level.getPath());
            }
        }
    }

    m_LevelList.clear();

    // Unload WADs
    if constexpr (appfw::isDebugBuild()) {
        for (WADAsset &wad : m_WADList) {
            for (WADMaterialAsset &mat : wad.m_Materials) {
                if (mat.m_uReferenceCount != 0) {
                    bLeaksFound = true;
                    std::string matName = "< unloaded >";

                    if (mat.getMaterial()) {
                        matName = mat.getMaterial()->getName();
                    }

                    printw("Material leaked: {}/{}", wad.getPath(), matName);
                }
            }
        }
    }

    m_WADList.clear();

    if (bLeaksFound) {
        printw("*** Asset leaks detected");
        AFW_ASSERT(false);
    }
}

void AssetManager::lateTick() {
    // Using late tick so during tick() pending assets are either available for all or for none.
    appfw::Timer timer;
    int maxTime = asset_max_tick.getValue();
    std::unique_lock lock(m_MainThreadTasksMutex);
    while (!m_MainThreadTasks.empty() && timer.ms() < maxTime) {
        // Get the next task
        AsyncTask task = std::move(m_MainThreadTasks.front());
        m_MainThreadTasks.pop();
        lock.unlock();

        // Run the task
        AFW_ASSERT(task);
        task();

        lock.lock();
    }
}

LevelAssetFuture AssetManager::loadLevel(std::string_view path) {
    printi("AssetManager: Loading level {}", path);

    LevelAssetFuture future;
    auto loadTask = std::packaged_task<LevelAssetRef()>(
        [this, path = std::string(path), state = future.initSharedState()]() {
            // Create a new level
            LevelAssetRef level;
            std::list<LevelAsset>::iterator it;
            {
                std::lock_guard lock(m_LevelMutex);
                it = m_LevelList.emplace(m_LevelList.end());
                level.set(&(*it));
            }

            // Load the level from disk
            try {
                level->loadFromFile(*this, path, *state);
            } catch (...) {
                // Remove the level
                level.reset();
                std::lock_guard lock(m_LevelMutex);
                m_LevelList.erase(it);
                throw;
            }

            return std::move(level);
        });

    future.setFuture(loadTask.get_future());
    addWorkerTask([task = std::move(loadTask)]() mutable { task(); });
    return future;
}

std::future<WADAssetRef> AssetManager::loadWADFile(std::string_view path) {
    auto loadTask = std::packaged_task<WADAssetRef()>([this, path = std::string(path)]() {
        return loadWADInternal(path);
    });

    std::future<WADAssetRef> future = loadTask.get_future();
    addWorkerTask([task = std::move(loadTask)]() mutable { task(); });
    return future;
}

WADMaterialAssetRef AssetManager::findMaterialByName(const char *szName) {
    std::lock_guard lock(m_WADMutex);

    for (const WADAsset &wad : m_WADList) {
        WADMaterialAssetRef mat = wad.findLoadedMaterial(szName);

        if (mat) {
            return mat;
        }
    }

    return nullptr;
}

WADAssetRef AssetManager::loadWADInternal(std::string_view path) {
    printi("AssetManager: Loading WAD {}", path);

    // Create a new entry
    WADAssetRef wad;
    std::list<WADAsset>::iterator it;
    {
        std::lock_guard lock(m_WADMutex);

        // See if this WAD is already loaded
        // It's possible if
        // Main thread:
        //   1. Begins level loading
        //   2. Begins WAD loading
        // Worker thread:
        //   1. Loads the level
        //   2. Loads the level WADs
        //   3. (main:2) requests one of the WADs loaded in (worker:2)
        // FIXME: Caseless compare?
        for (WADAsset &w : m_WADList) {
            if (w.getPath() == path) {
                // The WAD is already loaded
                AFW_ASSERT(w.isLoaded());
                return WADAssetRef(&w);
            }
        }

        it = m_WADList.emplace(m_WADList.end());
        wad.set(&(*it));
    }

    // Load the WAD from disk
    try {
        wad->loadFromFile(*this, path);
    } catch (...) {
        // Remove the WAD
        wad.reset();
        std::lock_guard lock(m_WADMutex);
        m_WADList.erase(it);
        throw;
    }

    return std::move(wad);
}

void AssetManager::addWorkerTask(AsyncTask &&task) {
    AFW_ASSERT(task);
    std::lock_guard lock(m_WorkerTasksMutex);
    m_WorkerTasks.push(std::move(task));
    m_WorkerTasksCondVar.notify_one();
}

void AssetManager::addMainThreadTask(AsyncTask &&task) {
    AFW_ASSERT(task);
    std::lock_guard lock(m_MainThreadTasksMutex);
    m_MainThreadTasks.push(std::move(task));
}

void AssetManager::workerThread() {
    try {
        while (m_bWorkerIsRunning) {
            std::unique_lock lock(m_WorkerTasksMutex);

            if (m_WorkerTasks.empty()) {
                m_WorkerTasksCondVar.wait(
                    lock, [&]() { return !m_bWorkerIsRunning || !m_WorkerTasks.empty(); });
            }

            if (!m_bWorkerIsRunning) {
                // App is shutting down
                return;
            }

            // Get the next task
            AsyncTask task = std::move(m_WorkerTasks.front());
            m_WorkerTasks.pop();
            lock.unlock();

            // Run the task
            AFW_ASSERT(task);
            task();
        }
    } catch (const std::exception &e) {
        app_fatalError("AssetManager::workerThread: {}", e.what());
    }
}

void AssetManager::fakeDelay() {
    int delay = (int)(asset_fake_delay.getValue() * 1000);
    if (delay > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
}
