#include <appfw/log/base_logger.h>
#include <appfw/dbg.h>

appfw::log::BaseLogger::BaseLogger() { m_MainThread = std::this_thread::get_id(); }

void appfw::log::BaseLogger::logStr(LogLevel level, const ModuleInfo &moduleInfo, const std::string &msg) {
    LogInfo info;
    info.time = std::time(nullptr);
    info.level = level;
    info.moduleInfo = &moduleInfo;
    
    if (std::this_thread::get_id() == m_MainThread) {
        log(info, msg + '\n');
    } else {
        LogMessage msg_struct;
        msg_struct.info = info;
        msg_struct.msg = msg + '\n';

        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_MsgQueue.push(std::move(msg_struct));
    }
}

void appfw::log::BaseLogger::processQueue() {
    std::lock_guard<std::mutex> lock(m_QueueMutex);
    AFW_ASSERT_MSG(m_MsgQueue.size() <= 100, "Msg queue is unusually long (> 100)");

    while (!m_MsgQueue.empty()) {
        LogMessage &msg = m_MsgQueue.front();
        log(msg.info, msg.msg);
        m_MsgQueue.pop();
    }
}
