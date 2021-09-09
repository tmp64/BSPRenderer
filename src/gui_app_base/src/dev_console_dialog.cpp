#include <appfw/appfw.h>
#include <gui_app_base/dev_console_dialog.h>

DevConsoleDialog::DevConsoleDialog() {
    ClearLog();
    memset(InputBuf, 0, sizeof(InputBuf));
    HistoryPos = -1;

    AutoScroll = true;
    ScrollToBottom = false;
}

DevConsoleDialog::~DevConsoleDialog() {
    ClearLog();
    for (int i = 0; i < History.Size; i++)
        free(History[i]);
}

int DevConsoleDialog::Stricmp(const char *s1, const char *s2) {
    int d;
    while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) {
        s1++;
        s2++;
    }
    return d;
}

int DevConsoleDialog::Strnicmp(const char *s1, const char *s2, int n) {
    int d = 0;
    while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) {
        s1++;
        s2++;
        n--;
    }
    return d;
}

char *DevConsoleDialog::Strdup(const char *s) {
    size_t len = strlen(s) + 1;
    void *buf = malloc(len);
    IM_ASSERT(buf);
    return (char *)memcpy(buf, (const void *)s, len);
}

void DevConsoleDialog::Strtrim(char *s) {
    char *str_end = s + strlen(s);
    while (str_end > s && str_end[-1] == ' ')
        str_end--;
    *str_end = 0;
}

void DevConsoleDialog::ClearLog() {
    for (int i = 0; i < Items.Size; i++)
        free(Items[i].text);
    Items.clear();
}

void DevConsoleDialog::AddLog(const char *fmt, ...) {
    // FIXME-OPT
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);
    Items.push_back({Strdup(buf), ImVec4(0, 0, 0, 0)});
}

void DevConsoleDialog::AddLog(ImVec4 color, const char *fmt, ...) {
    // FIXME-OPT
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);
    Items.push_back({Strdup(buf), color});
}

void DevConsoleDialog::Draw(const char *title, bool *p_open) {
    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }

    // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
    // So e.g. IsItemHovered() will return true when hovering the title bar.
    // Here we create a context menu only available from the title bar.
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Close Console"))
            *p_open = false;
        ImGui::EndPopup();
    }

    bool copy_to_clipboard = ImGui::SmallButton("Copy all to clipboard");

    ImGui::Separator();

    // Options menu
    if (ImGui::BeginPopup("Options")) {
        ImGui::Checkbox("Auto-scroll", &AutoScroll);
        ImGui::EndPopup();
    }

    // Options, Filter
    if (ImGui::Button("Options"))
        ImGui::OpenPopup("Options");
    ImGui::SameLine();
    Filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
    ImGui::Separator();

    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    if (ImGui::BeginPopupContextWindow()) {
        if (ImGui::Selectable("Clear"))
            ClearLog();
        ImGui::EndPopup();
    }

    // Display every line as a separate entry so we can change their color or add custom widgets.
    // If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
    // NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping
    // to only process visible items. The clipper will automatically measure the height of your first item and then
    // "seek" to display only items in the visible area.
    // To use the clipper we can replace your standard loop:
    //      for (int i = 0; i < Items.Size; i++)
    //   With:
    //      ImGuiListClipper clipper(Items.Size);
    //      while (clipper.Step())
    //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
    // - That your items are evenly spaced (same height)
    // - That you have cheap random access to your elements (you can access them given their index,
    //   without processing all the ones before)
    // You cannot this code as-is if a filter is active because it breaks the 'cheap random-access' property.
    // We would need random-access on the post-filtered list.
    // A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices
    // or offsets of items that passed the filtering test, recomputing this array when user changes the filter,
    // and appending newly elements as they are inserted. This is left as a task to the user until we can manage
    // to improve this example code!
    // If your items are of variable height:
    // - Split them into same height items would be simpler and facilitate random-seeking into your list.
    // - Consider using manual call to IsRectVisible() and skipping extraneous decoration from your items.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
    if (copy_to_clipboard)
        ImGui::LogToClipboard();

    {
        std::lock_guard lock(m_Mutex);

        for (int i = 0; i < Items.Size; i++) {
            const LogEntry &item = Items[i];
            if (!Filter.PassFilter(item.text))
                continue;

            bool has_color = item.color.w != 0;

            if (has_color)
                ImGui::PushStyleColor(ImGuiCol_Text, item.color);

            ImGui::TextUnformatted(item.text);

            if (has_color)
                ImGui::PopStyleColor();
        }
    }
    if (copy_to_clipboard)
        ImGui::LogFinish();

    if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        ImGui::SetScrollHereY(1.0f);
    ScrollToBottom = false;

    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();

    // Command-line
    bool reclaim_focus = false;
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue |
                                           ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub,
                         (void *)this)) {
        char *s = InputBuf;
        Strtrim(s);
        if (s[0])
            ExecCommand(s);
        strcpy(s, "");
        reclaim_focus = true;
    }

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

    ImGui::End();
}

void DevConsoleDialog::ExecCommand(const char *command_line) {
    appfw::ConMsgInfo msgInfo;
    msgInfo.setType(ConMsgType::Input).setTag("imgui console");
    m_pConSystem->print(msgInfo,
                        std::string("> ") + command_line);
    m_pConSystem->command(command_line);

    // Insert into history. First find match and delete it so it can be pushed to the back.
    // This isn't trying to be smart or optimal.
    HistoryPos = -1;
    for (int i = History.Size - 1; i >= 0; i--)
        if (Stricmp(History[i], command_line) == 0) {
            free(History[i]);
            History.erase(History.begin() + i);
            break;
        }
    History.push_back(Strdup(command_line));

    // On command input, we scroll to bottom even if AutoScroll==false
    ScrollToBottom = true;
}

int DevConsoleDialog::TextEditCallbackStub(ImGuiInputTextCallbackData *data) {
    DevConsoleDialog *console = (DevConsoleDialog *)data->UserData;
    return console->TextEditCallback(data);
}

int DevConsoleDialog::TextEditCallback(ImGuiInputTextCallbackData *data) {
    // AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
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

        auto &cmdMap = appfw::getConsole().getItemMap();
        for (auto &i : cmdMap) {
            if (Strnicmp(i.first.c_str(), word_start, (int)(word_end - word_start)) == 0) {
                candidates.push_back(i.first.c_str());
            }
        }

        if (candidates.Size == 0) {
            // No match
            AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
        } else if (candidates.Size == 1) {
            // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
            data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
            data->InsertChars(data->CursorPos, candidates[0]);
            data->InsertChars(data->CursorPos, " ");
        } else {
            // Multiple matches. Complete as much as we can..
            // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
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
            AddLog("Possible matches:\n");
            for (int i = 0; i < candidates.Size; i++)
                AddLog("- %s\n", candidates[i]);
        }

        break;
    }
    case ImGuiInputTextFlags_CallbackHistory: {
        // Example of HISTORY
        const int prev_history_pos = HistoryPos;
        if (data->EventKey == ImGuiKey_UpArrow) {
            if (HistoryPos == -1)
                HistoryPos = History.Size - 1;
            else if (HistoryPos > 0)
                HistoryPos--;
        } else if (data->EventKey == ImGuiKey_DownArrow) {
            if (HistoryPos != -1)
                if (++HistoryPos >= History.Size)
                    HistoryPos = -1;
        }

        // A better implementation would preserve the data on the current input line along with cursor position.
        if (prev_history_pos != HistoryPos) {
            const char *history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, history_str);
        }
    }
    }
    return 0;
}

void DevConsoleDialog::onAdd(appfw::ConsoleSystem *conSystem) {
    AFW_ASSERT(!m_pConSystem);
    m_pConSystem = static_cast <appfw::ConsoleSystem *>(conSystem);
    m_pConSystem->printPreviousMessages(this);
}

void DevConsoleDialog::onRemove([[maybe_unused]] appfw::ConsoleSystem *conSystem) {
    AFW_ASSERT(m_pConSystem == conSystem);
    m_pConSystem = nullptr;
}

void DevConsoleDialog::print(const appfw::ConMsgInfo &info, std::string_view msg) {
    std::lock_guard lock(m_Mutex);
    ConMsgColor color = info.color;
    if (color == ConMsgColor::Default) {
        color = appfw::getMsgTypeColor(info.type);
    }

    int colorInt = (int)color;

    int intensityBright = (colorInt & 0b1000) ? 255 : 128;
    int intensityDark = (colorInt & 0b1000) ? 96 : 64;
    ImColor conColor(
        (colorInt & 0b0100) ? intensityBright : intensityDark,
        (colorInt & 0b0010) ? intensityBright : intensityDark,
        (colorInt & 0b0001) ? intensityBright : intensityDark,
        255
    );

    AddLog(conColor, "%s", std::string(msg).c_str());
}

bool DevConsoleDialog::isThreadSafe() {
    return true;
}
