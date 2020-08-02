#ifndef APPFW_SERVICES_H
#define APPFW_SERVICES_H
#include <appfw/console/console_system.h>
#include <appfw/log/base_logger.h>
#include <appfw/dbg.h>
#include <appfw/module_info.h>
#include "this_module_info.h"

namespace appfw {

console::ConsoleSystem &getConsole();
log::BaseLogger &getLogger();

} // namespace appfw

namespace detail {

namespace MODULE_NAMESPACE {

//----------------------------------------------------------------
// Module info
//----------------------------------------------------------------
inline const appfw::ModuleInfo &getModuleInfo() {
    static appfw::ModuleInfo a{MODULE_NAME};
    return a;
}

//----------------------------------------------------------------
// Logging
//----------------------------------------------------------------
using LogLevel = appfw::log::LogLevel;

inline void vLogMsg(LogLevel level, const char *format, fmt::format_args args) {
    appfw::getLogger().logStr(level, getModuleInfo(), fmt::vformat(format, args));
}

template <typename... Args>
inline void logFatal(const char *format, const Args &... args) {
    vLogMsg(LogLevel::Fatal, format, fmt::make_format_args(args...));
}

template <typename... Args>
inline void logError(const char *format, const Args &... args) {
    vLogMsg(LogLevel::Error, format, fmt::make_format_args(args...));
}

template <typename... Args>
inline void logWarn(const char *format, const Args &... args) {
    vLogMsg(LogLevel::Warn, format, fmt::make_format_args(args...));
}

template <typename... Args>
inline void logInfo(const char *format, const Args &... args) {
    vLogMsg(LogLevel::Info, format, fmt::make_format_args(args...));
}

template <typename... Args>
inline void logDebug(const char *format, const Args &... args) {
    vLogMsg(LogLevel::Debug, format, fmt::make_format_args(args...));
}

//----------------------------------------------------------------
// Console
//----------------------------------------------------------------
using appfw::console::ConCommand;
using appfw::console::ConItemBase;
using appfw::console::ConsoleMsgType;
using appfw::console::ConVar;
using appfw::console::ConVarBase;

inline void vConPrint(ConsoleMsgType type, const char *format, fmt::format_args args) {
    appfw::console::ConsoleMsgInfo info(type);
    info.moduleName = MODULE_NAME;
    appfw::getConsole().print(info, fmt::vformat(format, args));
}

template <typename... Args>
inline void conPrint(const char *format, const Args &... args) {
    vConPrint(ConsoleMsgType::ConOutput, format, fmt::make_format_args(args...));
}

template <typename... Args>
inline void conPrint(ConsoleMsgType type, const char *format, const Args &... args) {
    vConPrint(type, format, fmt::make_format_args(args...));
}

} // namespace MODULE_NAMESPACE

} // namespace detail

using namespace detail::MODULE_NAMESPACE;

#endif