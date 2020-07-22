#ifndef APPFW_CONSOLE_STD_CONSOLE_H
#define APPFW_CONSOLE_STD_CONSOLE_H
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include <appfw/console/console_system.h>

namespace appfw {

namespace console {

class STDConsole : public IConsoleReceiver {
public:
    STDConsole();
    STDConsole(const STDConsole &) = delete;
    STDConsole &operator=(const STDConsole &) = delete;
    ~STDConsole();

    /**
     * Must be called every main loop tick, preferably before ConsoleSystem::processCommand.
     */
    void processQueue();

    virtual void onAdd(ConsoleSystem &conSystem) override;
    virtual void onRemove(ConsoleSystem &conSystem) override;
    virtual void print(const ConsoleMsgInfo &msgInfo, const std::string &msg) override;

private:
    struct SyncData {
        bool bIsValid;
        std::mutex mutex;
    };
    
    ConsoleSystem *m_pConSystem = nullptr;
    std::shared_ptr<SyncData> m_pSyncData;
    std::thread m_Thread;
    std::queue<std::string> m_CmdQueue;

    void setColorForType(int type);
    void resetColor();

    static void threadWorker(STDConsole *t, std::shared_ptr<SyncData> pSyncData);
};

} // namespace console

} // namespace appfw

#endif