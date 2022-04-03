#include "file_browser.h"
#include "hlviewer.h"

void FileBrowser::show(const char *name) {
    if (!ImGui::Begin(name)) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Refresh")) {
        invalidateCache();
    }

    showItems(m_RootCache, "");
    ImGui::End();
}

void FileBrowser::invalidateCache() {
    m_RootCache.invalidate();
}

void FileBrowser::showItems(DirCache &parent, const std::string &parentPath) {
    AFW_ASSERT_MSG(
        parentPath.empty() || (*parentPath.rbegin() == '/' && parentPath != "/"),
        "parentPath must end with a trailing slash unless it's empty, can't begin with a slash");

    if (!parent.isValid) {
        refreshCache(parent, parentPath);
    }

    // Directories first
    for (auto &[name, cache] : parent.dirs) {
        bool showNode = ImGui::TreeNode(name.c_str());

        if (ImGui::BeginPopupContextItem()) {
            showContextMenu(parentPath + name, true, true);
            ImGui::EndPopup();
        }

        if (showNode) {
            showItems(cache, fmt::format("{}{}/", parentPath, name));
            ImGui::TreePop();
        }
    }

    // Then files
    ImGui::Indent();
    for (auto &[name, cache] : parent.files) {
        if (!cache.isSupported) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(127, 127, 127, 255));
        }

        if (ImGui::Selectable(name.c_str())) {
            openFile(parentPath + name);
        }

        if (!cache.isSupported) {
            ImGui::PopStyleColor();
        }

        if (ImGui::BeginPopupContextItem()) {
            showContextMenu(parentPath + name, false, cache.isSupported);
            ImGui::EndPopup();
        }
    }
    ImGui::Unindent();
}

void FileBrowser::refreshCache(DirCache &dir, std::string path) {
    dir.invalidate();
    auto fileList = getFileSystem().getDirEntries(fmt::format("{}:{}", m_Tag, path));

    for (const auto &[name, dirEntry] : fileList) {
        if (dirEntry.is_directory()) {
            dir.dirs.insert({name, DirCache()});
        } else {
            FileCache cache;
            cache.isSupported = HLViewer::get().isFileSupported(name);
            dir.files.insert({name, std::move(cache)});
        }
    }

    dir.isValid = true;
}

void FileBrowser::showContextMenu(const std::string path, bool isDirectory, bool isSupported) {
    // NOTE: path doesn't have the tag

    if (!isDirectory) {
        ImGui::BeginDisabled(!isSupported);
        if (ImGui::Selectable("Open")) {
            openFile(path);
        }
        ImGui::EndDisabled();
    }

    if (ImGui::Selectable("Copy file name")) {
        size_t cutIdx = path.find_last_of('/');
        std::string name;

        if (cutIdx == path.npos) {
            name = path;
        } else {
            name = path.substr(cutIdx + 1);
        }

        ImGui::SetClipboardText(name.c_str());
    }

    if (ImGui::Selectable("Copy virtual path")) {
        ImGui::SetClipboardText(path.c_str());
    }

    if (ImGui::Selectable("Copy full path")) {
        std::string vpath = fmt::format("{}:{}", m_Tag, path);
        fs::path fullpath = getFileSystem().findExistingFile(vpath, std::nothrow);

        if (fullpath.empty()) {
            printe("Failed to copy the path of file '{}' since it is missing", vpath);
        } else {
            std::string fullpathstr = fullpath.u8string();

            if constexpr (appfw::isWindows()) {
                // Convert slashes into backslashes
                for (char &c : fullpathstr) {
                    if (c == '/') {
                        c = '\\';
                    }
                }
            }

            ImGui::SetClipboardText(fullpathstr.c_str());
        }
    }
}

void FileBrowser::openFile(std::string_view path) {
    HLViewer::get().openDialogForFile(fmt::format("{}:{}", m_Tag, path));
}

void FileBrowser::DirCache::invalidate() {
    isValid = false;
    dirs.clear();
    files.clear();
}
