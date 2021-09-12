#ifndef DEV_CONSOLE_DIALOG_H
#define DEV_CONSOLE_DIALOG_H
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <imgui.h>
#include <appfw/utils.h>
#include <appfw/console/console_system.h>

class DevConsoleDialog : appfw::NoMove, public appfw::IConsoleReceiver {
public:
    DevConsoleDialog();
    ~DevConsoleDialog();

    void tick();
    void clearLog();
    void printText(std::string_view msg);

    // IConsoleReceiver
    void onAdd(appfw::ConsoleSystem *conSystem) override;
    void onRemove(appfw::ConsoleSystem *conSystem) override;
    void print(const appfw::ConMsgInfo &info, std::string_view msg) override;
    bool isThreadSafe() override;

private:
    static constexpr size_t MAX_ITEM_COUNT = 1024;
    static constexpr size_t OVERLAY_ITEM_COUNT = 6;
    static constexpr float OVERLAY_OFFSET_X = 0.0f;
    static constexpr float OVERLAY_OFFSET_Y = 0.0f;
    static constexpr float OVERLAY_EXPIRATION_TIME = 5.0f;

    struct Item {
        ImVec4 dialogColor;
        ImVec4 overlayColor;
        std::string message;
        float overlayEndTime = 0;
        bool isExpired();
    };

    std::mutex m_Mutex;
    appfw::ConsoleSystem *m_pConSystem = nullptr;
    bool m_bAutoScroll = true;
    bool m_bScrollToBottom = false;

    // Item ring buffer
    std::vector<Item> m_Items;
    size_t m_ItemOffset = 0;

    // Input
    char m_InputBuf[512] = {};

    // History
    std::vector<std::string> m_History;
    int m_iHistoryPos = -1; // -1: new line, 0..History.Size-1 browsing history.

    void showDialog();
    void showOverlay();

    Item &getItem(size_t idx);
    ImVec4 getDialogItemColor(const appfw::ConMsgInfo &info);
    ImVec4 getOverlayItemColor(const appfw::ConMsgInfo &info);

    int textEditCallback(ImGuiInputTextCallbackData *data);
    void execCommand(const char *commandline);

    void drawOverlayTextShadow(Item &item);
    void drawOverlayText(Item &item);

    static int stricmp(const char *s1, const char *s2);
    static int strnicmp(const char *s1, const char *s2, int n);
    static void strtrim(char *s);
};

#endif
