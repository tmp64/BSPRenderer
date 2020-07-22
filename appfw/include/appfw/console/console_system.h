#ifndef APPFW_CONSOLE_CONSOLE_SYSTEM_H
#define APPFW_CONSOLE_CONSOLE_SYSTEM_H
#include <ctime>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <fmt/format.h>

#include <appfw/console/con_item.h>
#include <appfw/color.h>
#include <appfw/command_buffer.h>

namespace appfw {

namespace console {

class ConsoleSystem;

/**
 * Contains default types of console messages.
 * Receivers may also add custom ones.
 */
enum class ConsoleMsgType {
    // LogLevels.
    // Must match appfw::log::LogLevel
    Fatal = 0,  // Red
    Error = 1,  // Bright Red
    Warn = 2,   // Yellow
    Info = 3,   // Grey
    Debug = 4,  // Light Blue

    ConInput,       // Repeat of entered command - White
    ConOutput,      // Generic console output (e.g. form commands) - Grey
    ConOutputStrong,// Console output in a strong (bold or brighter) font.
    TypeCount
};

struct ConsoleMsgInfo {
    /**
     * UNIX type of the message.
     */
    time_t time = std::time(nullptr);

    /**
     * Type of the message.
     * @see ConsoleMsgType
     */
    int type = (int)ConsoleMsgType::ConOutput;

    /**
     * Color override.
     * If color.a() != 0, this color should be applied instead of default one.
     * Otherwise it must be ignored.
     * May not be supported by all receivers.
     */
    Color color;

    /**
     * Name of the module the message originates from.
     */
    const char *moduleName = "< unknown >";

    inline ConsoleMsgInfo(int _type, Color _color = Color()) {
        type = _type;
        color = _color;
    }

    /**
     * Default constructor.
     * ConOutput, no color.
     */
    inline ConsoleMsgInfo(ConsoleMsgType _type = ConsoleMsgType::ConOutput, Color _color = Color())
        : ConsoleMsgInfo((int)_type, _color) {}
};

/**
 * An interface for any console output dialog.
 */
class IConsoleReceiver {
public:
    virtual ~IConsoleReceiver() = default;

    /**
     * Called when this receiver was registered.
     * @arg conSystem Console system this was added to.
     */
    virtual void onAdd(ConsoleSystem &conSystem) = 0;

    /**
     * Called when this receiver was removed.
     * @arg conSystem Console system this was removed from.
     */
    virtual void onRemove(ConsoleSystem &conSystem) = 0;

    /**
     * Called to display a string to the console.
     */
    virtual void print(const ConsoleMsgInfo &msgInfo, const std::string &msg) = 0;
};

/**
 * Console System
 * Controls console variables, commands and console output.
 */
class ConsoleSystem {
public:
    using UnknownCmdHandler = std::function<void(const ParsedCommand &cmd)>;

    ConsoleSystem();
    ConsoleSystem(const ConsoleSystem &) = delete;
    ConsoleSystem &operator=(const ConsoleSystem &) = delete;
    ~ConsoleSystem();

    /**
     * Prints a string to the console.
     */
    void print(const ConsoleMsgInfo &msgInfo, const std::string &msg);

    /**
     * Finds an item by name.
     * @param   name    Name of the item.
     * @param   type    Type of the item (or Any).
     * @return Found item or nullptr.
     */
    ConItemBase *findItem(const std::string &name, ConItemType type = ConItemType::Any);

    /**
     * Finds a console variable by name.
     * @param   name    Name of the convar.
     * @return Found item or nullptr.
     */
    inline ConVarBase *findCvar(const std::string &name) {
        return static_cast<ConVarBase *>(findItem(name, ConItemType::ConVar));
    }

    /**
     * Finds a console command by name.
     * @param   name    Name of the command.
     * @return Found item or nullptr.
     */
    inline ConVarBase *findCommand(const std::string &name) {
        return static_cast<ConVarBase *>(findItem(name, ConItemType::ConCommand));
    }

    /**
     * Returns map of all items, ordered alphabetically.
     */
    const std::map<std::string, ConItemBase *> &getMap();

    /**
     * Registers all available ConItems in this system.
     * @return Number of registered items.
     */
    size_t registerAllAvailableItems();

    /**
     * Registers an item.
     */
    void registerItem(ConItemBase *pItem);

    /**
     * Parses `cmd` and adds it to the bottom of the queue.
     */
    void command(const std::string &cmd);

     /**
     * Adds `cmd` to the bottom of the queue.
     */
    void command(const ParsedCommand &cmd);

    /**
     * Executes specified command immediately.
     */
    void commandNow(const std::string &cmd);

    /**
     * Executes specified command immediately.
     */
    void commandNow(const ParsedCommand &cmd);

    /**
     * Executes a command at the top of the buffer.
     */
    void processCommand();

    /**
     * Executes all commands available in the buffer at the moment of call.
     */
    void processAllCommands();

    /**
     * Registers a console receiver to this console system.
     */
    void addConsoleReceiver(IConsoleReceiver *pRecv);

    /**
     * Removes a previously registered console receiver.
     */
    void removeConsoleReceiver(IConsoleReceiver *pRecv);

    /**
     * Specified function will be called in case a command couldn't be found.
     */
    void setUnknownCmdHandler(const UnknownCmdHandler &handler);

private:
    std::map<std::string, ConItemBase *> m_OrderedConItemMap;
    std::unordered_map<std::string, ConItemBase *> m_ConItemMap;
    std::unordered_set<ConItemBase *> m_ConItemList;
    std::unordered_set<IConsoleReceiver *> m_RecvList;
    CommandBuffer m_CmdBuffer;
    UnknownCmdHandler m_UnkCmdHandler;

    /**
     * Prints a formatted message to the console.
     * Sets up message info correctly.
     */
    void vPrintMsg(ConsoleMsgType msgType, const char *format, fmt::format_args args);

    template <typename... Args>
    inline void printMsg(ConsoleMsgType msgType, const char *format, const Args &... args) {
        vPrintMsg(msgType, format, fmt::make_format_args(args...));
    }

    /**
     * Default unknown command hander.
     * Prints "Unknown command" to the console.
     */
    void defaultUnkCmdHandler(const ParsedCommand &cmd);
};

} // namespace console

} // namespace appfw

#endif
