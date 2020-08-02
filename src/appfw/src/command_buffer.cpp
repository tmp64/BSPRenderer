#include <cassert>
#include <appfw/command_buffer.h>
#include <appfw/dbg.h>

appfw::CommandBuffer::CommandBuffer(const CommandHandler &handler) {
    setCommandHandler(handler);
}

void appfw::CommandBuffer::append(const std::string &cmd) {
    append(utils::parseCommand(cmd));
}

void appfw::CommandBuffer::append(const ParsedCommand &cmd) {
    if (cmd.size() >= 1) {
        m_CmdQueue.push(cmd);
    }
}

void appfw::CommandBuffer::executeOnce() {
    AFW_ASSERT_MSG(m_Handler, "Command buffer has no handler");

    if (!m_CmdQueue.empty()) {
        ParsedCommand &cmd = m_CmdQueue.front();
        m_Handler(cmd);
        m_CmdQueue.pop();
    }
}

void appfw::CommandBuffer::executeAll() {
    AFW_ASSERT_MSG(m_Handler, "Command buffer has no handler");
    
    size_t count = m_CmdQueue.size();
    for (size_t i = 0; i < count; i++) {
        ParsedCommand &cmd = m_CmdQueue.front();
        m_Handler(cmd);
        m_CmdQueue.pop();
    }
}

size_t appfw::CommandBuffer::getCommandCount() { return m_CmdQueue.size(); }

void appfw::CommandBuffer::setCommandHandler(const CommandHandler &handler) {
    m_Handler = handler;
}
