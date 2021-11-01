#include <fmt/printf.h>
#include <appfw/appfw.h>
#include <gui_app_base/imgui_controls.h>
#include <gui_app_base/gui_app_base.h>
#include "dev_console_dialog.h"

ConVar<bool> dev_console("dev_console", true, "Show developer console");
ConVar<bool> dev_console_overlay("dev_console_overlay", false, "Show developer console overlay");
static ConCommand cmd_toggleconsole("toggleconsole", "Toggle the developer console",
                                    []() { dev_console.setValue(!dev_console.getValue()); });

static KeyBind consoleKeyBind("Toggle developer console", KeyCode::Grave);

DevConsoleDialog::DevConsoleDialog() {
    m_Items.reserve(MAX_ITEM_COUNT);
    appfw::getConsole().addConsoleReceiver(this);

    dev_console.setCallback([](const bool &, const bool &newVal) {
        if (newVal && InputSystem::get().isInputGrabbed()) {
            // Ungrab input when console is opened
            InputSystem::get().setGrabInput(false);
        }

        return true;
    });

    cmd_toggleconsole.setCallback([]() {
        bool val = !dev_console.getValue();
        if (InputSystem::get().isInputGrabbed()) {
            // toggleconsole when grabbed should always show the console
            val = true;
        }
        dev_console.setValue(val);
    });
}

DevConsoleDialog::~DevConsoleDialog() {
    appfw::getConsole().removeConsoleReceiver(this);
}

void DevConsoleDialog::tick() {
    if (dev_console.getValue()) {
        showDialog();
    } else {
        if (consoleKeyBind.isPressed()) {
            dev_console.setValue(true);
        }
    }

    if (dev_console_overlay.getValue()) {
        showOverlay();
    }
}

void DevConsoleDialog::clearLog() {
    m_Items.clear();
    m_ItemOffset = 0;
}

void DevConsoleDialog::printText(std::string_view msg) {
    appfw::ConMsgInfo msgInfo;
    msgInfo.setTag("imgui console").setType(ConMsgType::Info);
    print(msgInfo, msg);
}

void DevConsoleDialog::onAdd(appfw::ConsoleSystem *conSystem) {
    AFW_ASSERT(!m_pConSystem);
    m_pConSystem = static_cast<appfw::ConsoleSystem *>(conSystem);
    m_pConSystem->printPreviousMessages(this);
}

void DevConsoleDialog::onRemove([[maybe_unused]] appfw::ConsoleSystem *conSystem) {
    AFW_ASSERT(m_pConSystem == conSystem);
    m_pConSystem = nullptr;
}

void DevConsoleDialog::print(const appfw::ConMsgInfo &info, std::string_view msg) {
    Item item;
    item.message = msg;
    item.dialogColor = getDialogItemColor(info);
    item.overlayColor = getOverlayItemColor(info);
    item.overlayEndTime = GuiAppBase::getBaseInstance().getTime() + OVERLAY_EXPIRATION_TIME;

    std::lock_guard lock(m_Mutex);

    if (m_Items.size() < MAX_ITEM_COUNT) {
        m_Items.push_back(std::move(item));
    } else {
        m_Items[m_ItemOffset] = std::move(item);
        m_ItemOffset = (m_ItemOffset + 1) % MAX_ITEM_COUNT;
    }
}

bool DevConsoleDialog::isThreadSafe() {
    return true;
}

void DevConsoleDialog::showDialog() {
    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);

    if (!ImGuiBeginCvar("Developer console", dev_console)) {
        ImGui::End();
        return;
    }

    bool copyToClipboard = ImGui::SmallButton("Copy all to clipboard");

    // Options menu
    if (ImGui::BeginPopup("Options")) {
        ImGui::Checkbox("Auto-scroll", &m_bAutoScroll);
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Options")) {
        ImGui::OpenPopup("Options");
    }

    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve =
        ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    {
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::Selectable("Clear"))
                clearLog();
            ImGui::EndPopup();
        }

        // Text
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

        if (copyToClipboard) {
            ImGui::LogToClipboard();
        }

        std::lock_guard lock(m_Mutex);
        ImGuiListClipper clipper;
        clipper.Begin((int)m_Items.size());

        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                Item &item = getItem(i);
                ImGui::PushStyleColor(ImGuiCol_Text, item.dialogColor);
                ImGui::TextUnformatted(item.message.c_str());
                ImGui::PopStyleColor();
            }
        }

        if (copyToClipboard) {
            ImGui::LogFinish();
        }

        if (m_bScrollToBottom || (m_bAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f);
        m_bScrollToBottom = false;

        ImGui::PopStyleVar();
        ImGui::EndChild();
    }

    ImGui::Separator();

    // Command line
    bool reclaimFocus = false;
    ImGuiInputTextFlags inputTextFlags = ImGuiInputTextFlags_EnterReturnsTrue |
                                         ImGuiInputTextFlags_CallbackCompletion |
                                         ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText(
            "Input", m_InputBuf, (int)std::size(m_InputBuf), inputTextFlags,
            [](ImGuiInputTextCallbackData *data) {
                DevConsoleDialog *console = (DevConsoleDialog *)data->UserData;
                return console->textEditCallback(data);
            },
            (void *)this)) {
        char *s = m_InputBuf;
        strtrim(s);
        if (s[0]) {
            execCommand(s);
        }
        m_InputBuf[0] = '\0';
        reclaimFocus = true;
    }

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaimFocus || ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget (input line)
    }

    if (consoleKeyBind.isPressed()) {
        if (!ImGui::IsWindowFocused()) {
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

    ImGui::End();
}

void DevConsoleDialog::showOverlay() {
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground;

    ImVec2 viewportSize = ImGui::GetMainViewport()->WorkSize;
    ImVec2 viewportPos = ImGui::GetMainViewport()->WorkPos;
    ImVec2 windowPos =
        ImVec2(viewportPos.x + OVERLAY_OFFSET_X * ImGui::GetFontSize(),
               viewportPos.y + viewportSize.y - OVERLAY_OFFSET_Y * ImGui::GetFontSize());
    ImVec2 windowPosPivot = ImVec2(0.0f, 1.0f);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);

    if (ImGui::Begin("Dev Console Overlay", nullptr, windowFlags)) {
        std::lock_guard lock(m_Mutex);
        ImVec2 startPos = ImGui::GetCursorStartPos();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

        size_t to = m_Items.size();
        size_t from = (to > OVERLAY_ITEM_COUNT) ? (to - OVERLAY_ITEM_COUNT) : 0;

        // Draw shadow
        ImGui::SetCursorPosY(startPos.y + 1);
        for (size_t i = from; i < to; i++) {
            ImGui::SetCursorPosX(startPos.x + 2);
            drawOverlayTextShadow(getItem(i));
        }

        // Draw text
        ImGui::SetCursorPos(startPos);
        for (size_t i = from; i < to; i++) {
            drawOverlayText(getItem(i));
        }

        ImGui::PopStyleVar();
    }

    ImGui::End();
}

DevConsoleDialog::Item &DevConsoleDialog::getItem(size_t idx) {
    AFW_ASSERT(idx < m_Items.size());
    size_t i = m_ItemOffset + idx;

    if (i >= m_Items.size()) {
        i -= m_Items.size();
    }

    return m_Items[i];
}

ImVec4 DevConsoleDialog::getDialogItemColor(const appfw::ConMsgInfo &info) {
    ConMsgColor color = info.color;
    if (color == ConMsgColor::Default) {
        color = appfw::getMsgTypeColor(info.type);
    }

    int colorInt = (int)color;

    int intensityBright = (colorInt & 0b1000) ? 255 : 128;
    int intensityDark = (colorInt & 0b1000) ? 96 : 48;
    return ImColor((colorInt & 0b0100) ? intensityBright : intensityDark,
                   (colorInt & 0b0010) ? intensityBright : intensityDark,
                   (colorInt & 0b0001) ? intensityBright : intensityDark, 255);
}

ImVec4 DevConsoleDialog::getOverlayItemColor(const appfw::ConMsgInfo &info) {
    ConMsgColor color = info.color;
    if (color == ConMsgColor::Default) {
        color = appfw::getMsgTypeColor(info.type);
    }

    int colorInt = (int)color;

    int intensityBright = 255;
    int intensityDark = 96;
    return ImColor((colorInt & 0b0100) ? intensityBright : intensityDark,
                   (colorInt & 0b0010) ? intensityBright : intensityDark,
                   (colorInt & 0b0001) ? intensityBright : intensityDark, 255);
}

int DevConsoleDialog::textEditCallback(ImGuiInputTextCallbackData *data) {
    switch (data->EventFlag) {
    case ImGuiInputTextFlags_CallbackCompletion: {
        // Example of TEXT COMPLETION

        // Locate beginning of current word
        const char *word_end = data->Buf + data->CursorPos;
        const char *word_start = word_end;
        while (word_start > data->Buf) {
            const char c = word_start[-1];
            if (c == ' ' || c == '\t' || c == ',' || c == ';')
                break;
            word_start--;
        }

        // Build a list of candidates
        ImVector<const char *> candidates;

        auto &cmdMap = m_pConSystem->getItemMap();
        for (auto &i : cmdMap) {
            if (strnicmp(i.first.c_str(), word_start, (int)(word_end - word_start)) == 0) {
                candidates.push_back(i.first.c_str());
            }
        }

        if (candidates.Size == 0) {
            // No match
            printText(fmt::sprintf("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start));
        } else if (candidates.Size == 1) {
            // Single match. Delete the beginning of the word and replace it entirely so we've got
            // nice casing.
            data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
            data->InsertChars(data->CursorPos, candidates[0]);
            data->InsertChars(data->CursorPos, " ");
        } else {
            // Multiple matches. Complete as much as we can..
            // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as
            // matches.
            int match_len = (int)(word_end - word_start);
            for (;;) {
                int c = 0;
                bool all_candidates_matches = true;
                for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                    if (i == 0)
                        c = toupper(candidates[i][match_len]);
                    else if (c == 0 || c != toupper(candidates[i][match_len]))
                        all_candidates_matches = false;
                if (!all_candidates_matches)
                    break;
                match_len++;
            }

            if (match_len > 0) {
                data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
                data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
            }

            // List matches
            printText("Possible matches:\n");
            for (int i = 0; i < candidates.Size; i++)
                printText(fmt::format("- {}\n", candidates[i]));
        }

        break;
    }
    case ImGuiInputTextFlags_CallbackHistory: {
        // Example of HISTORY
        const int prevHistoryPos = m_iHistoryPos;
        if (data->EventKey == ImGuiKey_UpArrow) {
            if (m_iHistoryPos == -1)
                m_iHistoryPos = (int)m_History.size() - 1;
            else if (m_iHistoryPos > 0)
                m_iHistoryPos--;
        } else if (data->EventKey == ImGuiKey_DownArrow) {
            if (m_iHistoryPos != -1)
                if (++m_iHistoryPos >= (int)m_History.size())
                    m_iHistoryPos = -1;
        }

        // A better implementation would preserve the data on the current input line along with
        // cursor position.
        if (prevHistoryPos != m_iHistoryPos) {
            const char *history_str = (m_iHistoryPos >= 0) ? m_History[m_iHistoryPos].c_str() : "";
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, history_str);
        }
    }
    }
    return 0;
}

void DevConsoleDialog::execCommand(const char *commandline) {
    appfw::ConMsgInfo msgInfo;
    msgInfo.setType(ConMsgType::Input).setTag("imgui console");
    m_pConSystem->print(msgInfo, std::string("> ") + commandline);
    m_pConSystem->command(commandline);

    // Insert into history. First find match and delete it so it can be pushed to the back.
    // This isn't trying to be smart or optimal.
    m_iHistoryPos = -1;
    for (int i = (int)m_History.size() - 1; i >= 0; i--)
        if (stricmp(m_History[i].c_str(), commandline) == 0) {
            m_History.erase(m_History.begin() + i);
            break;
        }
    m_History.push_back(commandline);

    // On command input, we scroll to bottom even if AutoScroll==false
    m_bScrollToBottom = true;
}

void DevConsoleDialog::drawOverlayTextShadow(Item &item) {
    if (item.isExpired())
        return;

    std::string &str = item.message;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
    ImGui::TextUnformatted(str.c_str(), str.c_str() + str.size());
    ImGui::PopStyleColor();
}

void DevConsoleDialog::drawOverlayText(Item &item) {
    if (item.isExpired())
        return;

    std::string &str = item.message;
    ImGui::PushStyleColor(ImGuiCol_Text, item.overlayColor);
    ImGui::TextUnformatted(str.c_str(), str.c_str() + str.size());
    ImGui::PopStyleColor();
}

int DevConsoleDialog::stricmp(const char *s1, const char *s2) {
    int d;
    while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) {
        s1++;
        s2++;
    }
    return d;
}

int DevConsoleDialog::strnicmp(const char *s1, const char *s2, int n) {
    int d = 0;
    while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) {
        s1++;
        s2++;
        n--;
    }
    return d;
}

void DevConsoleDialog::strtrim(char *s) {
    char *str_end = s + strlen(s);
    while (str_end > s && str_end[-1] == ' ')
        str_end--;
    *str_end = 0;
}

bool DevConsoleDialog::Item::isExpired() {
    return GuiAppBase::getBaseInstance().getTime() > overlayEndTime;
}
