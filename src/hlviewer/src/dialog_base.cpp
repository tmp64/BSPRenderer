#include "dialog_base.h"

DialogBase::DialogBase() {
    m_uUniqueId = m_suNextUniqueId++;
    m_WindowFlags |= ImGuiWindowFlags_NoSavedSettings;
}

DialogBase::~DialogBase() {}

void DialogBase::tick() {
    if (!m_bIsOpen) {
        return;
    }

    if (m_bBringToFront) {
        ImGui::SetNextWindowFocus();
        m_bBringToFront = false;
    }

    ImGui::SetNextWindowSize(ImVec2(m_DefaultSize.x, m_DefaultSize.y), ImGuiCond_Appearing);
    m_bContentVisible = ImGui::Begin(m_TitleID.c_str(), &m_bIsOpen, m_WindowFlags);

    if (m_bContentVisible) {
        showContents();
    }

    ImGui::End();
}

void DialogBase::preRender() {}

void DialogBase::setTitle(std::string_view title) {
    m_Title = title;
    m_TitleID = fmt::format("{}##dialog{}", title, m_uUniqueId);
}
