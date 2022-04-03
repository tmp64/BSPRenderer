#ifndef GRAPHICS_TEST_APP_H
#define GRAPHICS_TEST_APP_H
#include <gui_app_base/gui_app_base.h>
#include <hlviewer/assets/asset_manager.h>
#include "file_browser.h"
#include "dialog_base.h"

class HLViewer : public GuiAppBase {
public:
    using BaseClass = GuiAppBase;

    //! @returns the app instance.
    static inline HLViewer &get() { return static_cast<HLViewer &>(getBaseInstance()); }

    HLViewer();

    //! @returns average frame time.
    inline float getAvgFrameTime() { return m_flAvgFrameTime; }

    //! @param  filename    Name of the file (e.g. something.ext)
    //! @returns whether the file can be opened
    bool isFileSupported(std::string_view filename);

    //! Opens the dialog for specified file.
    DialogBase *openDialogForFile(std::string_view path);

protected:
    void tick() override;
    void drawBackground() override;

private:
    static constexpr float NEW_FRAME_TIME_RATIO = 0.02f;

    AssetManager m_AssetManager;
    FileBrowser m_FileBrowser;
    std::list<std::unique_ptr<DialogBase>> m_Dialogs;
    float m_flAvgFrameTime = 1 / 60.0f;

    void showDialogs();
};

#endif
