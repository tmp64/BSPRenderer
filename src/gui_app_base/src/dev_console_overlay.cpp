#include <gui_app_base/dev_console_overlay.h>
#include <gui_app_base/gui_app_base.h>

void DevConsoleOverlay::tick() {
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground;

    ImGuiIO &io = ImGui::GetIO();
    ImVec2 window_pos = ImVec2(OFFSET_X * ImGui::GetFontSize(),
                               io.DisplaySize.y - OFFSET_Y * ImGui::GetFontSize());
    ImVec2 window_pos_pivot = ImVec2(0.0f, 1.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

    if (ImGui::Begin("Dev Console Overlay", nullptr, window_flags)) {
        std::lock_guard lock(m_Mutex);
        ImVec2 startPos = ImGui::GetCursorStartPos();
        
        // Draw shadow
        ImGui::SetCursorPosY(startPos.y + 1);
        for (size_t i = m_uIdx; i < ITEM_COUNT; i++) {
            ImGui::SetCursorPosX(startPos.x + 2);
            drawTextShadow(m_Items[i]);
        }

        for (size_t i = 0; i < m_uIdx; i++) {
            ImGui::SetCursorPosX(startPos.x + 2);
            drawTextShadow(m_Items[i]);
        }

        // Draw text
        ImGui::SetCursorPos(startPos);
        for (size_t i = m_uIdx; i < ITEM_COUNT; i++) {
            drawText(m_Items[i]);
        }

        for (size_t i = 0; i < m_uIdx; i++) {
            drawText(m_Items[i]);
        }
    }

    ImGui::End();
}

void DevConsoleOverlay::onAdd(appfw::ConsoleSystem *) {}

void DevConsoleOverlay::onRemove(appfw::ConsoleSystem *) {}

void DevConsoleOverlay::print(const appfw::ConMsgInfo &info, std::string_view msg) {
    std::lock_guard lock(m_Mutex);
    Item &item = m_Items[m_uIdx];
    item.text = msg;
    item.color = getColor(info);
    item.endTime = GuiAppBase::getBaseInstance().getCurrentTime() + EXPIRATION_TIME;
    m_uIdx = (m_uIdx + 1) % ITEM_COUNT;
}

bool DevConsoleOverlay::isThreadSafe() {
    return true;
}

ImVec4 DevConsoleOverlay::getColor(const appfw::ConMsgInfo &info) {
    appfw::ConMsgColor color = info.color;

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

void DevConsoleOverlay::drawTextShadow(Item &item) {
    if (item.isExpired())
        return;

    std::string &str = item.text;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
    ImGui::TextUnformatted(str.c_str(), str.c_str() + str.size());
    ImGui::PopStyleColor();
}

void DevConsoleOverlay::drawText(Item &item) {
    if (item.isExpired())
        return;

    std::string &str = item.text;
    ImGui::PushStyleColor(ImGuiCol_Text, item.color);
    ImGui::TextUnformatted(str.c_str(), str.c_str() + str.size());
    ImGui::PopStyleColor();
}

bool DevConsoleOverlay::Item::isExpired() {
    return GuiAppBase::getBaseInstance().getCurrentTime() > endTime;
}
