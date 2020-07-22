#ifndef APPFW_LOG_CONSOLE_LOGGER_H
#define APPFW_LOG_CONSOLE_LOGGER_H
#include <appfw/log/base_logger.h>

namespace appfw {

namespace console {
class ConsoleSystem;
} // namespace console

namespace log {

class ConsoleLogger : public BaseLogger {
public:
    /**
     * @param   conSys  Output console system
     */
    ConsoleLogger(console::ConsoleSystem &conSys);

protected:
    virtual void log(const LogInfo &logInfo, const std::string &msg) override;

private:
    console::ConsoleSystem *m_pConSys = nullptr;
};

} // namespace log

} // namespace appfw

#endif