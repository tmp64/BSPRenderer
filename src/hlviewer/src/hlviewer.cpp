#include "hlviewer.h"
#include "map_viewer.h"

static ConCommand cmd_open("open", "Open a file", [](const CmdString &args) {
    if (args.size() < 2) {
        printi("Usage: open <virtual path to file>");
        return;
    }

    if (!HLViewer::get().openDialogForFile("assets:" + args[1])) {
        printe("Unsupported file.");
    }
});

const AppInitInfo &app_getInitInfo() {
    static AppInitInfo info = {"hlviewer", fs::u8path("")};
    return info;
}

std::unique_ptr<AppBase> app_createSingleton() {
    return std::make_unique<HLViewer>();
}

HLViewer::HLViewer() {
    m_FileBrowser.setTag("assets");
}

bool HLViewer::isFileSupported(std::string_view filename) {
    std::string_view extension;
    size_t extpos = filename.find_last_of('.');
    if (extpos != filename.npos) {
        extension = filename.substr(extpos + 1);
    }

    if (extension == "bsp") {
        return true;
    }

    return false;
}

DialogBase *HLViewer::openDialogForFile(std::string_view path) {
    std::unique_ptr<DialogBase> dialog;

    // Extract extension
    std::string_view extension;
    size_t extpos = path.find_last_of('.');
    if (extpos != path.npos) {
        extension = path.substr(extpos + 1);
    }

    if (extension == "bsp") {
        dialog = std::make_unique<MapViewer>(path);
    }

    if (!dialog) {
        return nullptr;
    }

    DialogBase *dialogPtr = dialog.get();
    m_Dialogs.push_back(std::move(dialog));
    // TODO: Sort them by title
    return dialogPtr;
}

void HLViewer::tick() {
    BaseClass::tick();

    // Update avg frametime
    m_flAvgFrameTime =
        NEW_FRAME_TIME_RATIO * getTimeDelta() + (1 - NEW_FRAME_TIME_RATIO) * m_flAvgFrameTime;

    ImGui::ShowDemoWindow();
    m_FileBrowser.show("File Browser");
    showDialogs();
}

void HLViewer::drawBackground() {
    BaseClass::drawBackground();

    for (auto &dialog : m_Dialogs) {
        dialog->preRender();
    }
}

void HLViewer::showDialogs() {
    auto it = m_Dialogs.begin();

    while (it != m_Dialogs.end()) {
        auto curItem = it;
        it++;

        if (!curItem->get()->isOpen()) {
            // Item ceased to exist
            m_Dialogs.erase(curItem);
        } else {
            curItem->get()->tick();
        }
    }
}
