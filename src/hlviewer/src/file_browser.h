#ifndef FILE_BROWSER_H
#define FILE_BROWSER_H
#include <appfw/appfw.h>
#include <imgui.h>

class FileBrowser {
public:
    //! Shows the dialog.
    void show(const char *name);

    //! Sets the tag for the file system.
    inline void setTag(std::string_view tag) { m_Tag = tag; }

    //! Clears all cached items.
    void invalidateCache();

private:
    struct FileCache {
        bool isSupported = false;
    };

    struct DirCache {
        bool isValid = false;
        std::map<std::string, DirCache> dirs;
        std::map<std::string, FileCache> files;

        //! Clears all cached data.
        void invalidate();
    };

    std::string m_Tag;
    DirCache m_RootCache;

    //! Shows items of a directory recursively.
    void showItems(DirCache &parent, const std::string &parentPath);

    //! Updates contents of a directory. Not recursive, only updates one level
    void refreshCache(DirCache &dir, std::string path);

    //! Shows context menu contents.
    void showContextMenu(const std::string path, bool isDirectory, bool isSupported);

    //! Opens the file.
    void openFile(std::string_view path);
};

#endif
