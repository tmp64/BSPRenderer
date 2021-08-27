#ifndef GUI_APP_DEV_CONSOLE_OVERLAY_H
#define GUI_APP_DEV_CONSOLE_OVERLAY_H
#include <array>
#include <string>
#include <mutex>
#include <imgui.h>
#include <appfw/console/console_system.h>

class DevConsoleOverlay : appfw::NoCopy, public appfw::IConsoleReceiver {
public:
    void tick();

    // IConsoleReceiver
    void onAdd(appfw::ConsoleSystem *conSystem) override;
    void onRemove(appfw::ConsoleSystem *conSystem) override;
    void print(const appfw::ConMsgInfo &info, std::string_view msg) override;
    bool isThreadSafe() override;

private:
    static constexpr size_t ITEM_COUNT = 4;
    static constexpr float OFFSET_X = 0.0f;
    static constexpr float OFFSET_Y = 0.0f;
    static constexpr double EXPIRATION_TIME = 5.0;

    struct Item {
        std::string text;
        ImVec4 color;
        double endTime = 0;
        
        bool isExpired();
    };

    std::mutex m_Mutex;
    std::array<Item, ITEM_COUNT> m_Items;
    size_t m_uIdx = 0;

    ImVec4 getColor(const appfw::ConMsgInfo &info);
    void drawTextShadow(Item &item);
    void drawText(Item &item);
};

#endif
