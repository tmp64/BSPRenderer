#include <iostream>

#include <Windows.h>

#include <appfw/console/std_console.h>
#include <appfw/dbg.h>

appfw::console::STDConsole::STDConsole() {
    resetColor();

    m_pSyncData = std::make_shared<SyncData>();
    m_pSyncData->bIsValid = true;
    m_Thread = std::thread(threadWorker, this, m_pSyncData);
    m_Thread.detach();
}

appfw::console::STDConsole::~STDConsole() {
    std::lock_guard<std::mutex> lock(m_pSyncData->mutex);
    m_pSyncData->bIsValid = false;
}

void appfw::console::STDConsole::processQueue() {
    std::lock_guard<std::mutex> lock(m_pSyncData->mutex);
    if (!m_CmdQueue.empty()) {
        const std::string &cmd = m_CmdQueue.front();
        print(ConsoleMsgInfo(ConsoleMsgType::ConInput), "> " + cmd + "\n");
        m_pConSystem->command(cmd);
        m_CmdQueue.pop();
    }
}

void appfw::console::STDConsole::onAdd(ConsoleSystem &conSystem) {
	AFW_ASSERT(!m_pConSystem);
    m_pConSystem = &conSystem;
}

void appfw::console::STDConsole::onRemove(ConsoleSystem &) {
	AFW_ASSERT(m_pConSystem);
    m_pConSystem = nullptr;
}

void appfw::console::STDConsole::print(const ConsoleMsgInfo &msgInfo, const std::string &msg) {
    setColorForType(msgInfo.type);
	std::cout << msg;
    resetColor();
}

void appfw::console::STDConsole::threadWorker(STDConsole *t, std::shared_ptr<SyncData> pSyncData) {
    for (;;) {
        std::string input;
        std::getline(std::cin, input);

        std::lock_guard<std::mutex> lock(pSyncData->mutex);

        if (!pSyncData->bIsValid) {
            // STDConsole no longer exists
            return;
        }

        t->m_CmdQueue.push(input);
    }
}

#ifdef _WIN32

void appfw::console::STDConsole::setColorForType(int itype) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    ConsoleMsgType type = (ConsoleMsgType)itype;
    WORD newColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

    switch (type) {
    case ConsoleMsgType::Fatal:
        newColor = FOREGROUND_RED;
        break;
    case ConsoleMsgType::Error:
        newColor = FOREGROUND_RED | FOREGROUND_INTENSITY;
        break;
    case ConsoleMsgType::Warn:
        newColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
    case ConsoleMsgType::Info:
        newColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
    case ConsoleMsgType::Debug:
        newColor = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
    case ConsoleMsgType::ConInput:
        newColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
    case ConsoleMsgType::ConOutput:
        newColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
    case ConsoleMsgType::ConOutputStrong:
        newColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
    }

    SetConsoleTextAttribute(h, newColor);
}

void appfw::console::STDConsole::resetColor() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

#else

void appfw::console::STDConsole::setColorForType(int type) {}

void appfw::console::STDConsole::resetColor() {}

#endif
