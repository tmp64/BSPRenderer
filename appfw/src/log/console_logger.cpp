#include <appfw/log/console_logger.h>
#include <appfw/console/console_system.h>

appfw::log::ConsoleLogger::ConsoleLogger(console::ConsoleSystem &conSys) { m_pConSys = &conSys; }

void appfw::log::ConsoleLogger::log(const LogInfo &logInfo, const std::string &msg) {
	console::ConsoleMsgInfo info;
    info.time = logInfo.time;
    info.type = (int)logInfo.level;
    m_pConSys->print(info, msg);
}
