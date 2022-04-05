#ifndef ERROR_DIALOG_H
#define ERROR_DIALOG_H
#include "dialog_base.h"

class ErrorDialog : public DialogBase {
public:
    ErrorDialog(std::string_view errorText, std::string_view title);

protected:
    void showContents() override;

private:
    static constexpr int WIDE = 800;
    static constexpr int TALL = 320;
    //static constexpr int WIDE_INNER = WIDE - 16;

    std::string m_ErrorText;
};

#endif
