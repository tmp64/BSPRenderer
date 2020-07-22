#ifndef APPFW_COMMAND_BUFFER_H
#define APPFW_COMMAND_BUFFER_H
#include <string>
#include <vector>
#include <queue>
#include <functional>

#include <appfw/utils.h>

namespace appfw {

/**
 * A queue of commands.
 */
class CommandBuffer {
public:
    using CommandHandler = std::function<void(const ParsedCommand &)>;

    CommandBuffer() = default;
    CommandBuffer(const CommandHandler &handler);

    /**
     * Parses `cmd` and adds it to the bottom of the queue.
     */
    void append(const std::string &cmd);

    /**
     * Adds `cmd` to the bottom of the queue.
     */
    void append(const ParsedCommand &cmd);

    /**
     * Executes a command at the top of the buffer.
     */
    void executeOnce();

    /**
     * Executes all commands available in the buffer at the moment of call.
     */
    void executeAll();

    /**
     * Returns numver of commands waiting to be executed.
     */
    size_t getCommandCount();

    /**
     * Sets a function that handles a command.
     * @param handler A function taking `const ParsedCommand &` to execute the command.
     */
    void setCommandHandler(const CommandHandler &handler);

private:
    std::queue<ParsedCommand> m_CmdQueue;
    CommandHandler m_Handler;
};

} // namespace appfw

#endif
