#include "error_dialog.h"

ErrorDialog::ErrorDialog(std::string_view errorText, std::string_view title) {
    if (!title.empty()) {
        setTitle(title);
    } else {
        setTitle("Error");
    }

    m_ErrorText = errorText;
    setDefaultSize({WIDE, TALL});
}

void ErrorDialog::showContents() {
    ImGui::TextWrapped("%s", m_ErrorText.c_str());
}
