#ifndef DEV_CONSOLE_DIALOG_H
#define DEV_CONSOLE_DIALOG_H
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <imgui.h>
#include <appfw/utils.h>
#include <appfw/console/console_system.h>

class DevConsoleDialog : appfw::NoCopy, public appfw::IConsoleReceiver {
public:
    DevConsoleDialog();
    ~DevConsoleDialog();

    // Portable helpers
    static int Stricmp(const char *s1, const char *s2);
    static int Strnicmp(const char *s1, const char *s2, int n);
    static char *Strdup(const char *s);
    static void Strtrim(char *s);

    void ClearLog();

    void AddLog(const char *fmt, ...);
    void AddLog(ImVec4 color, const char *fmt, ...);

    void Draw(const char *title, bool *p_open);

    void ExecCommand(const char *command_line);

    // In C++11 you'd be better off using lambdas for this sort of forwarding callbacks
    static int TextEditCallbackStub(ImGuiInputTextCallbackData *data);

    int TextEditCallback(ImGuiInputTextCallbackData *data);

    // IConsoleReceiver
    void onAdd(appfw::ConsoleSystem *conSystem) override;
    void onRemove(appfw::ConsoleSystem *conSystem) override;
    void print(const appfw::ConMsgInfo &info, std::string_view msg) override;
    bool isThreadSafe() override;

private:
    struct LogEntry {
        char *text;
        ImVec4 color;
    };

    char InputBuf[256];
    ImVector<LogEntry> Items;
    ImVector<char *> History;
    int HistoryPos; // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter Filter;
    bool AutoScroll;
    bool ScrollToBottom;
    appfw::ConsoleSystem *m_pConSystem = nullptr;
    std::mutex m_Mutex;
};

#endif
