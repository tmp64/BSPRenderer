#ifndef APPFW_LOG_BASE_LOGGER_H
#define APPFW_LOG_BASE_LOGGER_H
#include <ctime>
#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <appfw/module_info.h>

namespace appfw {

namespace log {

/**
 * All possible log levels.
 */
enum class LogLevel {
    Fatal = 0,
    Error,
    Warn,
    Info,
    Debug
};

struct LogInfo {
    LogLevel level;
    const ModuleInfo *moduleInfo;
    time_t time;
};

/**
 * A thread-safe module-aware logger.
 */
class BaseLogger {
public:
    BaseLogger();
    BaseLogger(const BaseLogger &) = delete;
    BaseLogger &operator=(const BaseLogger &) = delete;
    virtual ~BaseLogger() = default;

    /**
     * Logs a string.
     * Thread safe, but messages from other threads may be delayed.
     * @param   level         Log level of the message.
     * @param   moduleInfo    Module the message originates from.
     * @param   msg           Message
     */
    void logStr(LogLevel level, const ModuleInfo &moduleInfo, const std::string &msg);

    /**
     * Must be called from main thread to process messages from other threads.
     */
    void processQueue();

protected:
    /**
     * Logs msg to the output file/console/etc.
     * Always called from the main thread.
     */
    virtual void log(const LogInfo &logInfo, const std::string &msg) = 0;

private:
    struct LogMessage {
        LogInfo info;
        std::string msg;
    };

    std::thread::id m_MainThread;
    std::mutex m_QueueMutex;
    std::queue<LogMessage> m_MsgQueue;
};

} // namespace log

} // namespace appfw

#endif