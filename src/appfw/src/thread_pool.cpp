#include <stdexcept>
#include <appfw/services.h>
#include <appfw/thread_pool.h>

size_t appfw::ThreadDispatcher::getWorkCount() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_iWorkCount;
}

void appfw::ThreadDispatcher::setWorkCount(size_t workCount, size_t first) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_iWorkCount = workCount;
    m_iCurWork = first;
}

size_t appfw::ThreadDispatcher::getWork() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_iCurWork >= m_iWorkCount) {
        return WORK_DONE;
    } else {
        size_t work = m_iCurWork;
        m_iCurWork++;
        return work;
    }
}

appfw::ThreadPool::ThreadPool() {
    size_t threads = std::thread::hardware_concurrency();

	if (threads == 0) {
        setThreadCount(1);
	} else {
		setThreadCount(threads);
	}
}

appfw::ThreadPool::~ThreadPool() {
    m_bWorkerShutdown = true;

    for (std::thread &i : m_Threads) {
        i.join();
    }
}

size_t appfw::ThreadPool::getThreadCount() { return m_iThreadCount; }

void appfw::ThreadPool::setThreadCount(size_t count) {
    if (m_bThreadsSpawned) {
        throw std::logic_error("ThreadPool: can't change thread count after threads have been spawned");
    }
    m_iThreadCount = count;
}

bool appfw::ThreadPool::areThreadsSpawned() { return m_bThreadsSpawned; }

void appfw::ThreadPool::run(const WorkerFn &func, size_t iWorkSize = 0, void *pUserData) {
    spawnThreads();

    if (m_bWorkerRun) {
        throw std::logic_error("ThreadPool: still running");
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_WorkerFunction = func;
    m_WorkerWork = {iWorkSize, pUserData};
    m_bPoolFinished = false;

    for (ThreadInfo &i : m_ThreadInfo) {
        i.iWorkDone = 0;
        i.bIsDone = false;
    }

    std::this_thread::sleep_for(POLL_DELAY);
    std::this_thread::sleep_for(POLL_DELAY);
    std::this_thread::sleep_for(POLL_DELAY);
    m_bWorkerRun = true;
}

bool appfw::ThreadPool::isFinished() {
    for (ThreadInfo &i : m_ThreadInfo) {
        if (!i.bIsDone) {
            return false;
        }
    }

    if (m_bWorkerRun) {
        m_bWorkerRun = false;
        std::this_thread::sleep_for(POLL_DELAY);

        std::lock_guard<std::mutex> lock(m_Mutex);
        m_bPoolFinished = true;
        m_PoolFinishedCV.notify_all();

        std::this_thread::sleep_for(POLL_DELAY);
        std::this_thread::sleep_for(POLL_DELAY);
        std::this_thread::sleep_for(POLL_DELAY);
    }

    return true;
}

size_t appfw::ThreadPool::getStatus() { 
    size_t sum = 0;

    for (ThreadInfo &i : m_ThreadInfo) {
        sum += i.iWorkDone;
    }
    
    return sum;
}

bool appfw::ThreadPool::createThreadWork(size_t threadIdx, size_t totalWork, size_t &from, size_t &to) {
    if (getThreadCount() >= totalWork) {
        if (threadIdx >= totalWork) {
            from = 0;
            to = 0;
            return false;
        }

        from = threadIdx;
        to = threadIdx + 1;
        return true;
    }
    
    size_t workPerThread = totalWork / getThreadCount();

    from = workPerThread * threadIdx;

    if (threadIdx == getThreadCount() - 1) {
        to = totalWork;
    } else {
        to = from + workPerThread;
    }

    return true;
}

void appfw::ThreadPool::spawnThreads() {
    if (m_bThreadsSpawned) {
        return;
    }

    m_bThreadsSpawned = true;
    
    AFW_ASSERT(m_Threads.empty());
    AFW_ASSERT(m_ThreadInfo.empty());

    m_Threads.resize(m_iThreadCount);
    m_ThreadInfo.resize(m_iThreadCount);

    for (size_t i = 0; i < m_iThreadCount; i++) {
        ThreadInfo ti;
        ti.pPool = this;
        ti.iThreadIdx = i;
        m_ThreadInfo[i] = std::move(ti);
        m_Threads[i] = std::thread([this, i]() {
            internalThreadWorker(i);
        });
    }
}

void appfw::ThreadPool::internalThreadWorker(size_t threadIdx) {
    ThreadInfo &ti = m_ThreadInfo[threadIdx];
    
    for (;;) {
        if (m_bWorkerShutdown) {
            return;
        }

        if (!m_bWorkerRun) {
            std::this_thread::sleep_for(POLL_DELAY);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            ti.pWork = &m_WorkerWork;
            ti.iWorkDone = 0;
            ti.bIsDone = false;
        }

        try {
            m_WorkerFunction(ti);
        } catch (const std::exception &e) {
            logError("ThreadPool: unhandled exception from thread worker.");
            logError("ThreadPool: {}", e.what());
            std::abort();
        }

        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            ti.bIsDone = true;
        }

        /*while (m_bWorkerRun) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }*/
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_PoolFinishedCV.wait(lock, [&]() { return m_bPoolFinished.load(); });
        std::this_thread::sleep_for(POLL_DELAY);
    }
}
