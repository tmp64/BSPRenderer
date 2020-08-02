#include <appfw/console/console_system.h>
#include <appfw/dbg.h>
#include <appfw/services.h>

#include "this_module_info.h"

appfw::console::ConsoleSystem::ConsoleSystem() {
    m_CmdBuffer.setCommandHandler([this](const ParsedCommand &cmd) {
        commandNow(cmd);
    });

    setUnknownCmdHandler([this](const ParsedCommand &cmd) {
        defaultUnkCmdHandler(cmd);
    });
}

appfw::console::ConsoleSystem::~ConsoleSystem() {
    // Unregister all console receivers
    for (IConsoleReceiver *pRecv : m_RecvList) {
        pRecv->onRemove(*this);
    }

    m_RecvList.clear();
}

void appfw::console::ConsoleSystem::print(const ConsoleMsgInfo &msgInfo, const std::string &msg) {
    for (IConsoleReceiver *pRecv : m_RecvList) {
        pRecv->print(msgInfo, msg);
    }
}

appfw::console::ConItemBase *appfw::console::ConsoleSystem::findItem(const std::string &name, ConItemType type) {
    auto it = m_ConItemMap.find(name);

    if (it == m_ConItemMap.end()) {
        return nullptr;
    }

    ConItemType itemType = it->second->getType();

    if (type == ConItemType::Any || itemType == type) {
        return it->second;
    } else {
        return nullptr;
    }
}

const std::map<std::string, appfw::console::ConItemBase *> &appfw::console::ConsoleSystem::getMap() {
    return m_OrderedConItemMap;
}

size_t appfw::console::ConsoleSystem::registerAllAvailableItems() {
    auto &list = ConItemBase::getUnregItems();
    size_t count = 0;

    auto it = list.begin();
    while (it != list.end()) {
        registerItem(*it);
        list.pop_front();
        it = list.begin();
        count++;
    }

    return count;
}

void appfw::console::ConsoleSystem::registerItem(ConItemBase *pItem) {
    m_ConItemList.insert(pItem);
    m_ConItemMap.insert({pItem->getName(), pItem});
    m_OrderedConItemMap.insert({pItem->getName(), pItem});
}

void appfw::console::ConsoleSystem::command(const std::string &cmd) { m_CmdBuffer.append(cmd); }

void appfw::console::ConsoleSystem::command(const ParsedCommand &cmd) { m_CmdBuffer.append(cmd); }

void appfw::console::ConsoleSystem::commandNow(const std::string &cmd) { commandNow(utils::parseCommand(cmd)); }

void appfw::console::ConsoleSystem::commandNow(const ParsedCommand &cmd) {
    ConItemBase *pItem = findItem(cmd[0]);

    if (!pItem) {
        m_UnkCmdHandler(cmd);
        return;
    }

    ConItemType type = pItem->getType();

    if (type == ConItemType::ConVar) {
        ConVarBase *pCvar = static_cast<ConVarBase *>(pItem);

        if (cmd.size() == 1) {
            printMsg(ConsoleMsgType::ConOutput, "{}\n", pCvar->getDescr());
            printMsg(ConsoleMsgType::ConOutput, "    {} = \"{}\" ({}) {}\n", pCvar->getName(),
                     pCvar->getValueAsString(), pCvar->getVarTypeAsString(), pCvar->isLocked() ? "(locked)" : "");
        } else {
            VarSetResult result = pCvar->setValueString(cmd[1]);

            switch (result) {
            case VarSetResult::Success:
                printMsg(ConsoleMsgType::ConOutput, "Cvar set: {} = \"{}\"\n", pCvar->getName(),
                         pCvar->getValueAsString());
                break;
            case VarSetResult::InvalidString:
                printMsg(ConsoleMsgType::Error, "Failed to set cvar {}: invalid string value.\n", pCvar->getName());
                break;
            case VarSetResult::CallbackRejected:
                printMsg(ConsoleMsgType::Error, "Failed to set cvar {}: value rejected by callback.\n",
                         pCvar->getName());
                break;
            case VarSetResult::Locked:
                printMsg(ConsoleMsgType::Error, "Failed to set cvar {}: cvar is locked.\n",
                              pCvar->getName());
                break;
            }

        }
    } else if (type == ConItemType::ConCommand) {
        ConCommand *pCmd = static_cast<ConCommand *>(pItem);
        pCmd->execute(cmd);
    } else {
        AFW_ASSERT_MSG(false, "Unknown console item type");
    }
}

void appfw::console::ConsoleSystem::processCommand() { m_CmdBuffer.executeOnce(); }

void appfw::console::ConsoleSystem::processAllCommands() { m_CmdBuffer.executeAll(); }

void appfw::console::ConsoleSystem::addConsoleReceiver(IConsoleReceiver *pRecv) {
    AFW_ASSERT_MSG(m_RecvList.find(pRecv) == m_RecvList.end(), "pRecv has already been registered.");
    m_RecvList.insert(pRecv);
    pRecv->onAdd(*this);
}

void appfw::console::ConsoleSystem::removeConsoleReceiver(IConsoleReceiver *pRecv) {
    AFW_ASSERT_MSG(m_RecvList.find(pRecv) != m_RecvList.end(), "pRecv hasn't been registered.");
    pRecv->onRemove(*this);
    m_RecvList.erase(pRecv);
}

void appfw::console::ConsoleSystem::setUnknownCmdHandler(const UnknownCmdHandler &handler) {
    m_UnkCmdHandler = handler;
}

void appfw::console::ConsoleSystem::vPrintMsg(ConsoleMsgType msgType, const char *format, fmt::format_args args) {
    ConsoleMsgInfo info;
    info.moduleName = MODULE_NAME;
    info.type = (int)msgType;
    print(info, fmt::vformat(format, args));
}

void appfw::console::ConsoleSystem::defaultUnkCmdHandler(const ParsedCommand &cmd) {
    printMsg(ConsoleMsgType::ConOutput, "Unknown command: {}\n", cmd[0]);
}

//----------------------------------------------------------------

static void printItemInfo(ConItemBase *pItem) {
    using namespace appfw::console;

    ConItemType type = pItem->getType();

    if (type == ConItemType::ConVar) {
        ConVarBase *pCvar = static_cast<ConVarBase *>(pItem);
        conPrint(ConsoleMsgType::ConOutputStrong, "{} = \"{}\" ({}) {}\n", pCvar->getName(), pCvar->getValueAsString(),
                 pCvar->getVarTypeAsString(), pCvar->isLocked() ? "(locked)" : "");
    } else {
        conPrint(ConsoleMsgType::ConOutputStrong, "{}\n", pItem->getName());
    }
    conPrint("{}\n\n", pItem->getDescr());
}

static ConCommand s_List("list", "Lists all available console items.\nOptional: add 'cvar' or 'cmd' to filter.", [](const appfw::ParsedCommand &args) {
    using namespace appfw::console;

    ConItemType filter = ConItemType::Any;

    if (args.size() >= 2) {
        if (args[1] == "cvar") {
            filter = ConItemType::ConVar;
        } else if (args[1] == "cmd") {
            filter = ConItemType::ConCommand;
        } else if (args[1] == "all") {
            filter = ConItemType::Any;
        } else {
            conPrint("Usage: list [cvar|cmd|all]\n");
            conPrint("{}\n", appfw::getConsole().findCommand("list")->getDescr());
            return;
        }
    }

    auto &map = appfw::getConsole().getMap();
    size_t count = 0;

    for (auto &i : map) {
        if (filter == ConItemType::Any || filter == i.second->getType()) {
            printItemInfo(i.second);
            count++;
        }
    }

    conPrint("Total: {} item{}\n", count, count == 1 ? "" : "s");
                         });

static ConCommand s_Help("help", "Shows info about a command.", [](const appfw::ParsedCommand &args) {
    using namespace appfw::console;

    if (args.size() == 1) {
        conPrint("Usage: help <name>\n");
        return;
    }

    ConItemBase *pItem = appfw::getConsole().findItem(args[1]);

    if (!pItem) {
        conPrint(ConsoleMsgType::Error, "Error: item \"{}\" not found\n", args[1]);
        return;
    }

    printItemInfo(pItem);
});

//----------------------------------------------------------------

static ConCommand s_DebugLock("appfw_debug_lock_cvar", "Locks/unlocks a convar.", [](const appfw::ParsedCommand &args) {
    using namespace appfw::console;

    if (args.size() == 1) {
        conPrint("Usage: appfw_debug_lock_cvar <cvar name>\n");
        return;
    }

    ConVarBase *pCvar = appfw::getConsole().findCvar(args[1]);

    if (!pCvar) {
        conPrint(ConsoleMsgType::Error, "Error: cvar \"{}\" not found\n", args[1]);
        return;
    }

    pCvar->setLocked(!pCvar->isLocked());

    conPrint("Cvar \"{}\" is now {}\n", args[1], pCvar->isLocked() ? "locked" : "unlocked");
});
