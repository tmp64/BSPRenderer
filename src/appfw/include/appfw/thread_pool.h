#ifndef APPFW_THREAD_POOL_H
#define APPFW_THREAD_POOL_H
#include <atomic>
#include <chrono>
#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <appfw/utils.h>

namespace appfw {

class ThreadDispatcher {
public:
    static constexpr size_t WORK_DONE = std::numeric_limits<size_t>::max();

    /**
     * Returns value after maximum that can be returned by getWork.
     */
    size_t getWorkCount();

    /**
     * Resets the dispatcher and updates work count.
     */
    void setWorkCount(size_t workCount, size_t first = 0);

    /**
     * Returns work that should be processed next.
     */
    size_t getWork();

private:
    std::mutex m_Mutex;
    size_t m_iWorkCount = 0;
    size_t m_iCurWork = 0;
};

class ThreadPool : appfw::utils::NoCopy {
public:
    struct Work {
        size_t iWorkSize = 0;
        void *pUserParam = nullptr;
    };

    class ThreadInfo {
    public:
        ThreadPool *pPool = nullptr;
        size_t iThreadIdx = 0;
        const Work *pWork = nullptr;
        size_t iWorkDone = 0;

    private:
        bool bIsDone = false;

        friend class ThreadPool;
    };

    using WorkerFn = std::function<void(ThreadInfo &)>;

    static constexpr auto POLL_DELAY = std::chrono::milliseconds(50);

    ThreadPool();
    ~ThreadPool();

    /**
     * Returns amount of threads spawned or to be spawned.
     */
    size_t getThreadCount();

    /**
     * Sets amount of threads to be spawned.
     * Can only be called if areThreadsSpawned() == false.
     * Default: max num of CPU threads
     */
    void setThreadCount(size_t count);

    /**
     * Returns whether all threads have been spawned.
     */
    bool areThreadsSpawned();

    /**
     * Start workers.
     */
    void run(const WorkerFn &func, size_t iWorkSize, void *pUserData = nullptr);

    /**
     * Returns true if all threads have finished.
     */
    bool isFinished();

    /**
     * Returns sum of iWorkDone of all threads.
     */
    size_t getStatus();

    /**
     * Creates a range of work [from; to) based on total work and thread index.
     * Evenly distributes work between threads.
     * Return false if thread got no work.
     */
    bool createThreadWork(size_t threadIdx, size_t totalWork, size_t &from, size_t &to);

private:
    bool m_bThreadsSpawned = false;
    size_t m_iThreadCount = 0;
    std::vector<std::thread> m_Threads;
    std::vector<ThreadInfo> m_ThreadInfo;
    std::mutex m_Mutex;

    std::atomic_bool m_bWorkerShutdown = false; // On destruction
    std::atomic_bool m_bWorkerRun = false;
    std::atomic_bool m_bPoolFinished = false;
    std::condition_variable m_PoolFinishedCV;
    Work m_WorkerWork;
    WorkerFn m_WorkerFunction;

    void spawnThreads();

    void internalThreadWorker(size_t threadIdx);
};

}

#endif
